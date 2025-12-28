#pragma once
#include <string>
#include <unistd.h>
#include "ft_irc.h"

class Client
{
private:
	int fd;
	std::string ipAddress;
	std::string buffer;

	// Constructors and Destructors
	Client();

public:
	// Constructors and Destructors
	Client(const int fd, const std::string &ipadd);
	Client(const Client &copy);
	~Client();

	// Getters and Setters
	int getFd() const;
	std::string getIpAddress() const;

	void setFd(const int fd);
	void setIpAddress(const std::string &ipadd);

	// Functionality
	void addToBuffer(const std::string &str);

	// Operator Overloads
	const Client &operator=(const Client &copy);
};