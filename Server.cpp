#include "Server.hpp"
#include <cerrno>

Server::InitialisationError::InitialisationError(const std::string &message) : _message("Server Initialisation Error: " + message)
{
}

const char *Server::InitialisationError::what() const throw()
{
	return _message.c_str();
}

Server::InitialisationError::~InitialisationError() throw()
{
}

Server::Server(int port, const std::string &password) : _port(port), _socket_fd(-1), _stopRunning(false), _password(password)
{
}

Server::Server(const Server &copy)
{
	*this = copy;
}

void Server::Start()
{
	InitSocket();
	while (!_stopRunning)
	{
		try
		{
			int poll_count = 0;
			if (!_poll_fds.empty())
				poll_count = poll(&_poll_fds[0], _poll_fds.size(), 30000);
			if (poll_count < 0 && !_stopRunning)
				throw std::runtime_error("Poll failed");
			if (poll_count == 0)
			{
				pingClients();
				continue;
			}
			for (size_t i = 0; i < _poll_fds.size(); i++)
			{
				if ((_poll_fds[i].revents & POLLIN))
				{
					if (_poll_fds[i].fd == _socket_fd)
						AcceptNewClient();
					else
						ReceiveNewData(_poll_fds[i].fd);
				}
				if (_poll_fds[i].revents & (POLLHUP | POLLNVAL | POLLERR))
					_fdsToRemove.push_back(_poll_fds[i].fd);
			}
			for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
			{
				if (it->second)
					it->second->handleBuffer();
			}
			for (size_t i = 0; i < _fdsToRemove.size(); i++)
			{
				std::cout << "Client <" << _fdsToRemove[i] << "> Disconnected" << std::endl;
				removeClient(_fdsToRemove[i]);
			}
			_fdsToRemove.clear();
		}
		catch (const ClientError &e)
		{
			std::cerr << e.what() << '\n';
		}
		catch (...)
		{
			close(_socket_fd);
			removeAllClients();
			throw;
		}
	}
	std::cout << std::endl
			  << "Shutting down server elegently" << std::endl;
}

void Server::setStopRunning(bool value)
{
	_stopRunning = value;
}

const std::string Server::getName() const
{
	return "ric";
}

const std::string &Server::getPassword() const
{
	return _password;
}

const Client *Server::getClientByNick(const std::string &nick) const
{
	for (std::map<int, Client *>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second && it->second->getNickname() == nick)
			return it->second;
	}
	return NULL;
}

Client *Server::getClientByFd(int fd)
{
	std::map<int, Client *>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return NULL;
	return it->second;
}

const Client *Server::getClientByFd(int fd) const
{
	std::map<int, Client *>::const_iterator it = _clients.find(fd);
	if (it == _clients.end())
		return NULL;
	return it->second;
}

const Server &Server::operator=(const Server &copy)
{
	if (this == &copy)
		return *this;
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;
	_clients.clear();

	_address = copy._address;
	_port = copy._port;
	_password = copy._password;
	_socket_fd = copy._socket_fd;
	_stopRunning = copy._stopRunning;
	_poll_fds = copy._poll_fds;
	_fdsToRemove = copy._fdsToRemove;
	_channels = copy._channels;
	for (std::map<int, Client *>::const_iterator it = copy._clients.begin(); it != copy._clients.end(); ++it)
	{
		if (it->second)
			_clients[it->first] = new Client(*it->second);
	}

	return *this;
}

void Server::ReceiveNewData(int fd)
{
	char buff[1024] = {0};
	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

	if (bytes <= 0)
	{
		if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
			_fdsToRemove.push_back(fd);
	}
	else
	{
		buff[bytes] = '\0';
		Client *client = getClientByFd(fd);
		if (client)
		{
			std::cout << "Client <" << fd << "> Data: " << buff << std::endl;
			client->addToBuffer(std::string(buff));
		}
	}
}

void Server::queueRemoveClient(int fd)
{
	_fdsToRemove.push_back(fd);
}

void Server::removeClient(int fd)
{
	removeClientFromAllChannels(fd, "Client quit");
	for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it < _poll_fds.end(); it++)
		if (it->fd == fd)
		{
			_poll_fds.erase(it);
			break;
		}
	std::map<int, Client *>::iterator clientIt = _clients.find(fd);
	if (clientIt != _clients.end())
	{
		delete clientIt->second;
		_clients.erase(clientIt);
	}
	close(fd);
}

void Server::removeAllClients()
{
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	_clients.clear();
	_channels.clear();
	_poll_fds.clear();
}

void Server::InitSocket()
{
	int opt;
	_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket_fd == -1)
		throw InitialisationError("Socket Creation Failed");
	if (fcntl(_socket_fd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(_socket_fd);
		throw InitialisationError("Failed to set socket to non-blocking");
	}
	opt = 1;
	setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	if (bind(_socket_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
	{
		close(_socket_fd);
		throw InitialisationError("Binding Socket to Address Failed");
	}
	if (listen(_socket_fd, 10) < 0)
	{
		close(_socket_fd);
		throw InitialisationError("Failed to listen on socket");
	}

	struct pollfd server_pollfd;
	server_pollfd.fd = _socket_fd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;
	_poll_fds.push_back(server_pollfd);
}

void Server::AcceptNewClient()
{
	sockaddr_in client_address;
	socklen_t client_len = sizeof(client_address);

	int client_socket = accept(_socket_fd, (struct sockaddr *)&client_address, &client_len);
	if (client_socket < 0)
		throw ClientError("Failed to accept new client");
	if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1)
	{
		close(client_socket);
		throw ClientError("Failed Setting Client Socket to Non-Blocking");
	}
	pollfd client_pollfd;
	client_pollfd.fd = client_socket;
	client_pollfd.events = POLLIN;
	client_pollfd.revents = 0;
	_poll_fds.push_back(client_pollfd);

	Client *newClient = new Client(client_socket, inet_ntoa(client_address.sin_addr));
	_clients.insert(std::make_pair(client_socket, newClient));
	std::cout << "Accepted Client with address: " << newClient->getIpAddress() << std::endl;
}

static std::string toLower(const std::string &str)
{
	std::string result = str;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = std::tolower(result[i]);
	return result;
}

void Server::sendToClient(int fd, const std::string &message)
{
	Client *client = getClientByFd(fd);
	if (client)
		client->sendToClient(message);
}

Channel *Server::getChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = _channels.find(toLower(name));
	if (it == _channels.end())
		return NULL;
	return &(it->second);
}

const Channel *Server::getChannel(const std::string &name) const
{
	std::map<std::string, Channel>::const_iterator it = _channels.find(toLower(name));
	if (it == _channels.end())
		return NULL;
	return &(it->second);
}

Channel &Server::createChannel(const std::string &name, int creatorFd)
{
	std::string lower = toLower(name);
	std::pair<std::map<std::string, Channel>::iterator, bool> result = _channels.insert(std::make_pair(lower, Channel(lower)));
	result.first->second.addMember(creatorFd);
	result.first->second.addOperator(creatorFd);
	return result.first->second;
}

void Server::broadcastToChannel(const std::string &name, const std::string &message, int exceptFd)
{
	Channel *channel = getChannel(toLower(name));
	if (!channel)
		return;
	const std::set<int> &members = channel->getMembers();
	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		if (*it == exceptFd)
			continue;
		sendToClient(*it, message);
	}
}

void Server::removeChannelIfEmpty(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = _channels.find(toLower(name));
	if (it != _channels.end() && it->second.memberCount() == 0)
		_channels.erase(it);
}

void Server::removeClientFromAllChannels(int fd, const std::string &reason)
{
	Client *client = getClientByFd(fd);
	if (!client)
		return;
	std::vector<std::string> channelsToErase;
	std::string nick = client->getNickname();
	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (!it->second.hasMember(fd))
			continue;
		std::string message = ":" + nick + " QUIT :" + reason;
		broadcastToChannel(it->first, message, fd);
		it->second.removeMember(fd);
		if (it->second.memberCount() == 0)
			channelsToErase.push_back(it->first);
	}
	for (size_t i = 0; i < channelsToErase.size(); ++i)
		_channels.erase(channelsToErase[i]);
}

void Server::pingClients()
{
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client *client = it->second;
		if (!client)
			continue;
		if (client->hasPendingPing())
			_fdsToRemove.push_back(it->first);
		else
			client->sendPing();
	}
	for (size_t i = 0; i < _fdsToRemove.size(); i++)
	{
		std::cout << "Client <" << _fdsToRemove[i] << "> timed out" << std::endl;
		removeClient(_fdsToRemove[i]);
	}
	_fdsToRemove.clear();
}

Server::~Server()
{
	if (_socket_fd >= 0)
		close(_socket_fd);
	removeAllClients();
}

Server::ClientError::ClientError(const std::string &message) : _message("Client Error: " + message)
{
}

const char *Server::ClientError::what() const throw()
{
	return _message.c_str();
}

Server::ClientError::~ClientError() throw()
{
}
