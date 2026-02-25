#include "Server.hpp"

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

Server::Server(int port, const std::string &password) : _port(port), _password(password)
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
			int poll_count = poll(_poll_fds.data(), _poll_fds.size(), -1);
			if (poll_count < 0 && !_stopRunning)
				throw std::runtime_error("Poll failed");
			for (size_t i = 0; i < _poll_fds.size(); i++)
			{
				if ((_poll_fds[i].revents & POLLIN))
				{
					if (_poll_fds[i].fd == _socket_fd)
						AcceptNewClient();
					else
						ReceiveNewData(_poll_fds[i].fd);
				}
				if (_poll_fds[i].revents & (POLLHUP | POLLNVAL))
				{
					_fdsToRemove.push_back(_poll_fds[i].fd);
				}
			}
			for (size_t i = 0; i < _fdsToRemove.size(); i++)
			{
				removeClient(_fdsToRemove[i]);
				std::cout << "Client <" << _fdsToRemove[i] << "> Disconnected" << std::endl;
			}
			_fdsToRemove.clear();
			for (size_t i = 0; i < _clients.size(); i++)
			{
				_clients[i].handleBuffer();
			}
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

const Server &Server::operator=(const Server &copy)
{
	if (this == &copy)
		return *this;

	_address = copy._address;
	_port = copy._port;
	_password = copy._password;
	_socket_fd = copy._socket_fd;

	return *this;
}

void Server::ReceiveNewData(int fd)
{
	// TODO check if theres a way to do this with more than 1024
	char buff[1024] = {0};
	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

	if (bytes <= 0)
		_fdsToRemove.push_back(fd);
	else
	{
		buff[bytes] = '\0';
		for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->getFd() == fd)
			{
				std::cout << "Client <" << fd << "> Data: " << buff << std::endl;
				it->addToBuffer(std::string(buff));
				break;
			}
		}
	}
}

void Server::removeClient(int fd)
{
	for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it < _poll_fds.end(); it++)
		if (it->fd == fd)
		{
			_poll_fds.erase(it);
			break;
		}
	for (std::vector<Client>::iterator it = _clients.begin(); it < _clients.end(); it++)
		if (it->getFd() == fd)
		{
			_clients.erase(it);
			break;
		}
	close(fd);
}

void Server::removeAllClients()
{
	for (std::vector<Client>::iterator it = _clients.begin(); it < _clients.end(); it++)
		close(it->getFd());
	_clients.clear();
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

	Client newClient(client_socket, inet_ntoa(client_address.sin_addr));
	_clients.push_back(newClient);
	std::cout << "Accepted Client with address: " << newClient.getIpAddress() << std::endl;
}

void Server::sendToClient(int fd, const std::string &message)
{
	for (std::vector<Client>::iterator it = _clients.begin(); it < _clients.end(); it++)
		if (it->getFd() == fd)
		{
			it->sendToClient(message);
		}
}

Server::~Server()
{
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
