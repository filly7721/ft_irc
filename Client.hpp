#pragma once
#include <string>
#include <unistd.h>
#include "ft_irc.h"

struct Command;

typedef enum e_numeric
{
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONEUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTRED = 462,
	ERR_PASSWDMISMATCH = 464,
	ERR_NOSUCHCHANNEL = 403,
	ERR_NOTONCHANNEL = 442,
	ERR_INVITEONLYCHAN = 473,
	ERR_CHANNELISFULL = 471,
	ERR_BADCHANNELKEY = 475,
	ERR_NOSUCHNICK = 401,
	ERR_NOTREGISTERED = 451,
	ERR_NORECIPIENT = 411,
	ERR_NOTEXTTOSEND = 412,
	ERR_USERNOTINCHANNEL = 441,
	ERR_USERONCHANNEL = 443,
	ERR_CHANOPRIVSNEEDED = 482,
	ERR_UNKNOWNMODE = 472,
	RPL_CHANNELMODEIS = 324,
	RPL_INVITING = 341,
	RPL_NOTOPIC = 331,
	RPL_TOPIC = 332,
	RPL_NAMREPLY = 353,
	RPL_ENDOFNAMES = 366,
} t_numeric;
class Client
{
public:
	// Constructors and Destructors
	Client(const int fd, const std::string &ipadd);
	Client(const Client &copy);
	~Client();

	// Getters and Setters
	int getFd() const;
	std::string getIpAddress() const;
	std::string getNickname() const;
	std::string getUsername() const;

	void setFd(const int fd);
	void setIpAddress(const std::string &ipadd);
	void setNickname(const std::string &nickname);

	// Functionality
	void addToBuffer(const std::string &str);
	void sendToClient(const std::string &str);
	void sendNumeric(const t_numeric numeric, const std::string &message);
	void handleBuffer();
	void handleCommand(const Command &command);
	void tryRegister();
	void sendPing();
	void resetPing();
	bool hasPendingPing() const;
	bool isValidNickname(const std::string &nick);

	// Commands
	void cmdCAP(const Command &cmd);
	void cmdNick(const Command &cmd);
	void cmdPass(const Command &cmd);
	void cmdUser(const Command &cmd);
	void cmdPong(const Command &cmd);
	void cmdQuit(const Command &cmd);
	void cmdPrivmsg(const Command &cmd);
	void cmdJoin(const Command &cmd);
	void cmdPart(const Command &cmd);
	void cmdKick(const Command &cmd);
	void cmdInvite(const Command &cmd);
	void cmdTopic(const Command &cmd);
	void cmdMode(const Command &cmd);

	// Operator Overloads
	Client &operator=(const Client &copy);

private:
	// Default Constructor
	Client();

private:
	// Connection
	int _fd;
	std::string _ipAddress;
	std::string _buffer;

	// Identity
	std::string _nickname;
	std::string _username;
	std::string _realname;

	// State
	bool _isRegistered;
	bool _isAuthenticated;
	bool _pendingPing;
};