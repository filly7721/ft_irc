#pragma once
#include <string>
#include <unistd.h>
#include "Server.hpp"
#include "ft_irc.h"

struct Command;

typedef enum e_numeric
{
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONEUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTRED = 462,
} t_numeric;
class Client
{
private:
	int _fd;
	std::string _ipAddress;
	std::string _buffer;

	std::string _nickname;
	std::string _username;
	std::string _realname;

	bool _isRegistered;
	bool _isAuthenticated;

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
	void sendToClient(const std::string &str);
	void sendNumeric(const t_numeric numeric, const std::string &message);
	void handleBuffer();
	bool isValidNickname(const std::string &nick);

	// Commands
	void cmdCAP(const Command &cmd);
	void cmdNick(const Command &cmd);
	void cmdPass(const Command &cmd);
	void cmdUser(const Command &cmd);

	// Operator Overloads
	const Client &operator=(const Client &copy);
};