#include "Client.hpp"

Client::Client(const int fd, const std::string &ipadd) : fd(fd), ipAddress(ipadd)
{
}

Client::Client(const Client &copy)
{
	*this = copy;
}

Client::~Client()
{
}

int Client::getFd() const
{
	return fd;
}

void Client::setFd(const int fd)
{
	this->fd = fd;
}

void Client::setIpAddress(const std::string &ipadd)
{
	ipAddress = ipadd;
}

void Client::addToBuffer(const std::string &str)
{
	buffer += str;
}

void Client::handleBuffer()
{
	size_t pos;
	while ((pos = buffer.find("\n")) != std::string::npos)
	{
		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 1);
		Command command = parseMessage(line);
	}
}

const Client &Client::operator=(const Client &copy)
{
	if (this == &copy)
		return *this;

	fd = copy.fd;
	ipAddress = copy.ipAddress;
	buffer = copy.buffer;

	return *this;
}

std::string Client::getIpAddress() const
{
	return ipAddress;
}