#include "Client.hpp"

bool Client::isValidNickname(const std::string &nick)
{
	if (nick.empty() || nick.length() > 9)
	{
		return false;
	}

	for (size_t i = 0; i < nick.length(); ++i)
	{
		char c = nick[i];
		if (i == 0)
		{
			if (!std::isalpha(c) && c != '_')
			{
				return false;
			}
		}
		else
		{
			if (!std::isalnum(c) && c != '_' && c != '-')
			{
				return false;
			}
		}
	}

	return true;
}

Client::Client(const int fd, const std::string &ipadd) : _fd(fd), _ipAddress(ipadd), _isRegistered(false), _isAuthenticated(false)
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
	return _fd;
}

void Client::setFd(const int fd)
{
	this->_fd = fd;
}

void Client::setIpAddress(const std::string &ipadd)
{
	_ipAddress = ipadd;
}

void Client::addToBuffer(const std::string &str)
{
	_buffer += str;
}

void Client::sendToClient(const std::string &str)
{
	const std::string toSend = str + "\r\n";
	send(_fd, toSend.data(), toSend.size(), 0);
}

void Client::sendNumeric(const t_numeric numeric, const std::string &message)
{
	std::ostringstream oss;
	oss << ':' << g_server->getName() << ' ' << numeric << ' ' << _nickname << ' ' << message;
	sendToClient(oss.str());
}

void Client::handleBuffer()
{
	size_t pos;
	while ((pos = _buffer.find("\n")) != std::string::npos)
	{
		std::string line = _buffer.substr(0, pos);
		_buffer.erase(0, pos + 1);
		Command command = parseMessage(line);
		if (command.name == "CAP")
			cmdCAP(command);
		else if (command.name == "NICK")
			cmdNick(command);
		else if (command.name == "PASS")
			cmdPass(command);
		else if (command.name == "USER")
			cmdUser(command);
	}
}

void Client::cmdNick(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendNumeric(ERR_NONICKNAMEGIVEN, "No nickname given");
		return;
	}
	if (!isValidNickname(cmd.params[0]))
		sendNumeric(ERR_ERRONEUSNICKNAME, "Nickname not valid");
	// if (cmd.params[0] == "used")
	// 	sendNumeric(ERR_NICKNAMEINUSE);

	std::string oldNick = _nickname;
	_nickname = cmd.params[0];
	if (!oldNick.empty())
	{
		std::string message = std::string(":") + oldNick + " NICK " + _nickname;
		sendToClient(message);
	}
}

void Client::cmdPass(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "No password given");
		return;
	}
	if (_isRegistered)
	{
		sendNumeric(ERR_ALREADYREGISTRED, "Already registered");
		return;
	}
	if (g_server->getPassword() != cmd.params[0])
	{
		// maybe send err idk
		return;
	}
	_isAuthenticated = true;
}

void Client::cmdUser(const Command &cmd)
{
	if (cmd.params.size() < 4)
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "Missing params");
		return;
	}
	if (_isRegistered)
	{
		sendNumeric(ERR_ALREADYREGISTRED, "Already registered");
		return;
	}
	_username = cmd.params[0];
	_realname = cmd.params[3];
	if (_isAuthenticated && _nickname != "" && _username != "" && _realname != "")
	{
		_isRegistered = true;
		sendNumeric((t_numeric)001, "Welcome to the Internet Relay Network " + _nickname + "!" + _username + "@" + _ipAddress);
		sendNumeric((t_numeric)002, "Your host is " + g_server->getName() + ", running version 1.0");
		sendNumeric((t_numeric)003, "This server was created today");
		sendNumeric((t_numeric)004, g_server->getName() + " 1.0 o itkol");
		sendNumeric((t_numeric)005, "CHANTYPES=# PREFIX=(o)@ CHANLIMIT=#:10 CHANNELLEN=50 NICKLEN=9 NETWORK=" + g_server->getName() + " : are supported by this server");
		sendNumeric((t_numeric)375, ": - ircserv Message of the day - ");
		sendNumeric((t_numeric)372, ": - Welcome to the IRC server!");
		sendNumeric((t_numeric)372, ": - This server is ready for use.");
		sendNumeric((t_numeric)376, ": End of /MOTD command");
	}
}

void Client::cmdCAP(const Command &cmd)
{
	if (cmd.params.empty())
		return;
	if (cmd.params[0] == "LS")
	{
		std::string store = "ircserv CAP " + (_nickname.empty() ? "*" : _nickname) + " LS :\r\n";
		g_server->sendToClient(_fd, store);
	}
}

const Client &Client::operator=(const Client &copy)
{
	if (this == &copy)
		return *this;

	_fd = copy._fd;
	_ipAddress = copy._ipAddress;
	_buffer = copy._buffer;

	return *this;
}

std::string Client::getIpAddress() const
{
	return _ipAddress;
}