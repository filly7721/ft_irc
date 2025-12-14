#pragma once
#include <string>

// TODO Make Orthodox Canonical Form
class Client
{
private:
	int Fd;
	std::string IPadd;

public:
	Client() {};
	Client(int fd, std::string ipadd) : Fd(fd), IPadd(ipadd) {}

	int GetFd() const { return Fd; }
	void SetFd(int fd) { Fd = fd; }
	void setIpAdd(std::string ipadd) { IPadd = ipadd; }
	std::string getIpAdd() const { return IPadd; }
};