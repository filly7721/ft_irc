#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <poll.h>
#include <stdexcept>
#include <arpa/inet.h>
#include <iostream>
#include "Client.hpp"

class Server
{
public:
	// Constructors and Destructors
	Server(int port);
	Server(const Server &copy);
	~Server();

	// Functionality
	void Start();

	// operator overloads
	const Server &operator=(const Server &copy);

public:
	// Exception Classes
	class InitialisationError : public std::exception
	{
	private:
		std::string _message;
		InitialisationError();

	public:
		InitialisationError(const std::string &message);
		const char *what() const throw();
		~InitialisationError() throw();
	};
	class ClientError : public std::exception
	{
	private:
		std::string _message;
		ClientError();

	public:
		ClientError(const std::string &message);
		const char *what() const throw();
		~ClientError() throw();
	};

private:
	// Default Constructor
	Server();

	// Functionality
	void InitSocket();
	void AcceptNewClient();
	void ReceiveNewData(int fd);
	void removeClient(int fd);
	void removeAllClients();

	// Signal Handler
	static void handleSignal(int signal);

private:
	// Network Values
	sockaddr_in _address;
	in_port_t _port;
	int _socket_fd;
	int _opt;

	// Clients and Fds
	std::vector<struct pollfd> _poll_fds;
	std::vector<class Client> _clients;
	std::vector<int> _fdsToRemove;

	// Static Control Flag
	static bool _stopRunning;
};
