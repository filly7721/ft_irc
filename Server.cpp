#include "Server.hpp"

Server::InitialisationError::InitialisationError(std::string message) : _message(message)
{
}

const char *Server::InitialisationError::what() const throw()
{
	return ("Server Initialisation Error: " + _message).c_str();
}

Server::InitialisationError::~InitialisationError() throw()
{
}

Server::Server(int port) : _port(port), _opt(1)
{
}

Server::Server(const Server &copy)
{
	*this = copy;
}

void Server::Start()
{
	_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket_fd == 0)
		throw InitialisationError("Socket Creation Failed");
	if (fcntl(_socket_fd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(_socket_fd);
		throw InitialisationError("Failed to set socket to non-blocking");
	}

	setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &_opt, sizeof(_opt));

	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	if (bind(_socket_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		throw InitialisationError("Binding Socket to Address Failed");

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

	while (true)
	{
		// TODO Try Catch Runtime Errors
		int poll_count = poll(&_poll_fds[0], _poll_fds.size(), -1);
		if (poll_count < 0)
		{
			close(_socket_fd);
			throw std::runtime_error("Poll failed"); // TODO Custom Exception
		}
		for (size_t i = 0; i < _poll_fds.size(); i++)
		{
			if ((_poll_fds[i].revents & POLLIN) != 0)
			{
				if (_poll_fds[i].fd == _socket_fd)
					AcceptNewClient();
				else
					ReceiveNewData(_poll_fds[i].fd);
			}
		}
	}
}
const Server &Server::operator=(const Server &copy)
{
	if (this == &copy)
		return *this;

	_address = copy._address;
	_port = copy._port;
	_socket_fd = copy._socket_fd;
	_opt = copy._opt;

	return *this;
}
void Server::ReceiveNewData(int fd)
{
	char buff[1024];
	memset(buff, 0, sizeof(buff));
	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

	if (bytes <= 0)
	{
		// TODO Implement queue to remove clients outside of iteration loop
		std::cout << "Client <" << fd << "> Disconnected" << std::endl;
		close(fd);
		removeClient(fd);
	}
	else
	{
		buff[bytes] = '\0';
		std::cout << "Client <" << fd << "> Data: " << buff;
	}
}

void Server::removeClient(int fd)
{
	for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it < _poll_fds.end(); it++)
		if (it->fd == fd)
			_poll_fds.erase(it);
	for (std::vector<Client>::iterator it = _clients.begin(); it < _clients.end(); it++)
		if (it->GetFd() == fd)
			_clients.erase(it);
}

void Server::AcceptNewClient()
{
	sockaddr_in client_address;
	socklen_t client_len = sizeof(client_address);

	int client_socket = accept(_socket_fd, (struct sockaddr *)&client_address, &client_len);
	if (client_socket < 0)
	{
		close(_socket_fd);
		throw std::runtime_error("Failed to accept new client"); // TODO Create Custom Exception and Catch in Main Loop
	}
	if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1)
	{
		close(client_socket);
		throw std::runtime_error("Failed Setting Client Socket to Non-Blocking"); // TODO Create Custom Exception and Catch in Main Loop
	}
	pollfd client_pollfd;
	client_pollfd.fd = client_socket;
	client_pollfd.events = POLLIN;
	client_pollfd.revents = 0;
	_poll_fds.push_back(client_pollfd);

	Client newClient(client_socket, inet_ntoa(client_address.sin_addr));
	_clients.push_back(newClient);
	std::cout << "Accepted Client with address: " << newClient.getIpAdd() << std::endl;
}

Server::~Server() {}
