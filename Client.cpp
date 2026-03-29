#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include <sys/socket.h>

std::vector<std::string> splitByComma(const std::string &value)
{
	std::vector<std::string> parts;
	std::string current;
	for (size_t i = 0; i < value.size(); ++i)
	{
		if (value[i] == ',')
		{
			parts.push_back(current);
			current.clear();
			continue;
		}
		current += value[i];
	}
	parts.push_back(current);
	return parts;
}

std::string buildPrefix(const Client &client)
{
	std::string nick = client.getNickname().empty() ? "*" : client.getNickname();
	std::string user = client.getUsername().empty() ? "*" : client.getUsername();
	return nick + "!" + user + "@" + client.getIpAddress();
}

typedef std::pair<std::string, void (Client::*)(const Command &)> CommandEntry;

static std::vector<CommandEntry> makePublicCmds()
{
	std::vector<CommandEntry> v;
	v.push_back(CommandEntry("CAP",  &Client::cmdCAP));
	v.push_back(CommandEntry("PASS", &Client::cmdPass));
	v.push_back(CommandEntry("NICK", &Client::cmdNick));
	v.push_back(CommandEntry("USER", &Client::cmdUser));
	v.push_back(CommandEntry("QUIT", &Client::cmdQuit));
	return v;
}

static std::vector<CommandEntry> makePrivateCmds()
{
	std::vector<CommandEntry> v;
	v.push_back(CommandEntry("PRIVMSG", &Client::cmdPrivmsg));
	v.push_back(CommandEntry("JOIN",    &Client::cmdJoin));
	v.push_back(CommandEntry("PART",    &Client::cmdPart));
	v.push_back(CommandEntry("KICK",    &Client::cmdKick));
	v.push_back(CommandEntry("INVITE",  &Client::cmdInvite));
	v.push_back(CommandEntry("TOPIC",   &Client::cmdTopic));
	v.push_back(CommandEntry("MODE",    &Client::cmdMode));
	return v;
}

static const std::vector<CommandEntry> PublicCommands = makePublicCmds();
static const std::vector<CommandEntry> PrivateCommands = makePrivateCmds();

void Client::handleCommand(const Command &command)
{
	for (size_t i = 0; i < PublicCommands.size(); ++i)
	{
		if (PublicCommands[i].first == command.name)
		{
			(this->*PublicCommands[i].second)(command);
			return;
		}
	}
	if (!_isRegistered)
	{
		sendNumeric(ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	for (size_t i = 0; i < PrivateCommands.size(); ++i)
	{
		if (PrivateCommands[i].first == command.name)
		{
			(this->*PrivateCommands[i].second)(command);
			return;
		}
	}
}

bool Client::isValidNickname(const std::string &nick)
{
	if (nick.empty() || nick.length() > 9)
		return false;

	for (size_t i = 0; i < nick.length(); ++i)
	{
		char c = nick[i];
		if (i == 0)
		{
			if (!std::isalpha(c) && c != '_')
				return false;
		}
		else if (!std::isalnum(c) && c != '_' && c != '-')
				return false;
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

void Client::setNickname(const std::string &nickname)
{
	_nickname = nickname;
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
	oss << ':' << g_server->getName() << ' ' << numeric << ' '
	    << (_nickname.empty() ? "*" : _nickname) << ' ' << message;
	sendToClient(oss.str());
}

void Client::handleBuffer()
{
	size_t pos;
	while ((pos = _buffer.find('\n')) != std::string::npos)
	{
		std::string line = _buffer.substr(0, pos);
		_buffer.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		Command command = parseMessage(line);

		handleCommand(command);
	}
}

void Client::cmdCAP(const Command &cmd)
{
	if (cmd.params.empty())
		return;
	if (cmd.params[0] == "LS")
	{
		std::string store = "ircserv CAP " + (_nickname.empty() ? "*" : _nickname) + " LS :";
		g_server->sendToClient(_fd, store);
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
	{
		sendNumeric(ERR_ERRONEUSNICKNAME, "Nickname not valid");
		return;
	}
	const Client *existing = g_server->getClientByNick(cmd.params[0]);
	if (existing && existing->getFd() != _fd)
	{
		sendNumeric(ERR_NICKNAMEINUSE, cmd.params[0] + " :Nickname is already in use");
		return;
	}
	std::string oldNick = _nickname;
	_nickname = cmd.params[0];
	if (!oldNick.empty())
		sendToClient(":" + oldNick + " NICK :" + _nickname);
	tryRegister();
}

void Client::cmdPass(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
		return;
	}
	if (_isRegistered)
	{
		sendNumeric(ERR_ALREADYREGISTRED, "You may not reregister");
		return;
	}
	if (g_server->getPassword() != cmd.params[0])
	{
		sendNumeric(ERR_PASSWDMISMATCH, ":Password incorrect");
		return;
	}
	_isAuthenticated = true;
}

void Client::cmdUser(const Command &cmd)
{
	if (cmd.params.size() < 4)
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
		return;
	}
	if (_isRegistered)
	{
		sendNumeric(ERR_ALREADYREGISTRED, "You may not reregister");
		return;
	}
	_username = cmd.params[0];
	_realname = cmd.params[3];
	tryRegister();
}

void Client::tryRegister()
{
	if (_isRegistered || !_isAuthenticated || _nickname.empty() || _username.empty() || _realname.empty())
		return;
	_isRegistered = true;
	sendNumeric((t_numeric)001, ":Welcome to the Internet Relay Network " + _nickname + "!" + _username + "@" + _ipAddress);
	sendNumeric((t_numeric)002, ":Your host is " + g_server->getName() + ", running version 1.0");
	sendNumeric((t_numeric)003, ":This server was created today");
	sendNumeric((t_numeric)004, g_server->getName() + " 1.0 o itkol");
	sendNumeric((t_numeric)005, "CHANTYPES=# PREFIX=(o)@ CHANLIMIT=#:10 CHANNELLEN=50 NICKLEN=9 NETWORK=" + g_server->getName() + " :are supported by this server");
	sendNumeric((t_numeric)375, ":- ircserv Message of the day -");
	sendNumeric((t_numeric)372, ":- Welcome to the IRC server!");
	sendNumeric((t_numeric)372, ":- This server is ready for use.");
	sendNumeric((t_numeric)376, ":End of /MOTD command");
}

void Client::cmdPrivmsg(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendNumeric(ERR_NORECIPIENT, "PRIVMSG :No recipient given");
		return;
	}
	if (cmd.params.size() < 2 || cmd.params[1].empty())
	{
		sendNumeric(ERR_NOTEXTTOSEND, ":No text to send");
		return;
	}
	std::string target = cmd.params[0];
	std::string message = cmd.params[1];
	if (target.find_first_of(',') != std::string::npos)
	{
		sendNumeric(ERR_NORECIPIENT, "PRIVMSG :Multiple recipients not supported");
		return;
	}
	if (!target.empty() && target[0] == '#')
	{
		Channel *channel = g_server->getChannel(target);
		if (!channel)
		{
			sendNumeric(ERR_NOSUCHCHANNEL, target + " :No such channel");
			return;
		}
		if (!channel->hasMember(_fd))
		{
			sendNumeric(ERR_NOTONCHANNEL, target + " :You're not on that channel");
			return;
		}
		g_server->broadcastToChannel(target, ":" + buildPrefix(*this) + " PRIVMSG " + target + " :" + message, _fd);
		return;
	}
	const Client *recipient = g_server->getClientByNick(target);
	if (!recipient)
	{
		sendNumeric(ERR_NORECIPIENT, "PRIVMSG :No recipient given");
		return;
	}
	g_server->sendToClient(recipient->getFd(), ":" + buildPrefix(*this) + " PRIVMSG " + target + " :" + message);
}

void Client::cmdJoin(const Command &cmd)
{
	if (cmd.params.empty() || cmd.params[0].empty())
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
		return;
	}
	std::vector<std::string> channels = splitByComma(cmd.params[0]);
	std::vector<std::string> keys;
	if (cmd.params.size() > 1)
		keys = splitByComma(cmd.params[1]);
	for (size_t index = 0; index < channels.size(); ++index)
	{
		const std::string &channelName = channels[index];
		if (channelName.empty() || channelName[0] != '#')
		{
			sendNumeric(ERR_NOSUCHCHANNEL, channelName + " :No such channel");
			continue;
		}
		const std::string key = (index < keys.size()) ? keys[index] : "";
		Channel *channel = g_server->getChannel(channelName);
		if (!channel)
			channel = &g_server->createChannel(channelName, _fd);
		if (channel->hasMember(_fd))
			continue;
		if (channel->isInviteOnly() && !channel->isInvited(_fd))
		{
			sendNumeric(ERR_INVITEONLYCHAN, channelName + " :Cannot join channel (+i)");
			continue;
		}
		if (!channel->getKey().empty() && channel->getKey() != key)
		{
			sendNumeric(ERR_BADCHANNELKEY, channelName + " :Cannot join channel (+k)");
			continue;
		}
		if (channel->isFull())
		{
			sendNumeric(ERR_CHANNELISFULL, channelName + " :Cannot join channel (+l)");
			continue;
		}
		channel->addMember(_fd);
		channel->removeInvite(_fd);
		std::string joinMsg = ":" + buildPrefix(*this) + " JOIN :" + channelName;
		g_server->broadcastToChannel(channelName, joinMsg, -1);
		if (channel->getTopic().empty())
			sendNumeric(RPL_NOTOPIC, channelName + " :No topic is set");
		else
			sendNumeric(RPL_TOPIC, channelName + " :" + channel->getTopic());
		std::string names;
		const std::set<int> &members = channel->getMembers();
		for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
		{
			const Client *memberClient = g_server->getClientByFd(*it);
			if (!memberClient)
				continue;
			if (!names.empty())
				names += " ";
			if (channel->isOperator(*it))
				names += "@";
			names += memberClient->getNickname();
		}
		sendNumeric(RPL_NAMREPLY, "= " + channelName + " :" + names);
		sendNumeric(RPL_ENDOFNAMES, channelName + " :End of /NAMES list");
	}
}

void Client::cmdPart(const Command &cmd)
{
	if (cmd.params.empty() || cmd.params[0].empty())
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return;
	}
	std::vector<std::string> channels = splitByComma(cmd.params[0]);

	for (size_t index = 0; index < channels.size(); ++index)
	{
		const std::string &channelName = channels[index];
		Channel *channel = g_server->getChannel(channelName);
		if (!channel)
		{
			sendNumeric(ERR_NOSUCHCHANNEL, channelName + " :No such channel");
			continue;
		}
		if (!channel->hasMember(_fd))
		{
			sendNumeric(ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
			continue;
		}
		std::string partMsg = ":" + buildPrefix(*this) + " PART " + channelName;
		g_server->broadcastToChannel(channelName, partMsg, -1);
		channel->removeMember(_fd);
		g_server->removeChannelIfEmpty(channelName);
	}
}

Client &Client::operator=(const Client &copy)
{
	if (this == &copy)
		return *this;

	_fd = copy._fd;
	_ipAddress = copy._ipAddress;
	_buffer = copy._buffer;
	_nickname = copy._nickname;
	_username = copy._username;
	_realname = copy._realname;
	_isRegistered = copy._isRegistered;
	_isAuthenticated = copy._isAuthenticated;

	return *this;
}

std::string Client::getIpAddress() const
{
	return _ipAddress;
}

std::string Client::getNickname() const
{
	return _nickname;
}

std::string Client::getUsername() const
{
	return _username;
}

void Client::cmdQuit(const Command &cmd)
{
	std::string reason = cmd.params.empty() ? "Client quit" : cmd.params[0];
	g_server->removeClientFromAllChannels(_fd, reason);
	g_server->queueRemoveClient(_fd);
}

void Client::cmdKick(const Command &cmd)
{
	if (cmd.params.size() < 2)
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
		return;
	}
	const std::string &channelName = cmd.params[0];
	const std::string &targetNick = cmd.params[1];
	const std::string reason = (cmd.params.size() > 2) ? cmd.params[2] : targetNick;

	Channel *channel = g_server->getChannel(channelName);
	if (!channel)
	{
		sendNumeric(ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return;
	}
	if (!channel->hasMember(_fd))
	{
		sendNumeric(ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(_fd))
	{
		sendNumeric(ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}
	const Client *target = g_server->getClientByNick(targetNick);
	if (!target || !channel->hasMember(target->getFd()))
	{
		sendNumeric(ERR_USERNOTINCHANNEL, targetNick + " " + channelName + " :They aren't on that channel");
		return;
	}
	std::string kickMsg = ":" + buildPrefix(*this) + " KICK " + channelName + " " + targetNick + " :" + reason;
	g_server->broadcastToChannel(channelName, kickMsg, -1);
	channel->removeMember(target->getFd());
	g_server->removeChannelIfEmpty(channelName);
}

void Client::cmdInvite(const Command &cmd)
{
	if (cmd.params.size() < 2)
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
		return;
	}
	const std::string &targetNick = cmd.params[0];
	const std::string &channelName = cmd.params[1];

	Channel *channel = g_server->getChannel(channelName);
	if (!channel)
	{
		sendNumeric(ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return;
	}
	if (!channel->hasMember(_fd))
	{
		sendNumeric(ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(_fd))
	{
		sendNumeric(ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}
	const Client *target = g_server->getClientByNick(targetNick);
	if (!target)
	{
		sendNumeric(ERR_NOSUCHNICK, targetNick + " :No such nick");
		return;
	}
	if (channel->hasMember(target->getFd()))
	{
		sendNumeric(ERR_USERONCHANNEL, targetNick + " " + channelName + " :is already on channel");
		return;
	}
	channel->addInvite(target->getFd());
	sendNumeric(RPL_INVITING, channelName + " " + targetNick);
	g_server->sendToClient(target->getFd(), ":" + buildPrefix(*this) + " INVITE " + targetNick + " :" + channelName);
}

void Client::cmdTopic(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
		return;
	}
	const std::string &channelName = cmd.params[0];
	Channel *channel = g_server->getChannel(channelName);
	if (!channel)
	{
		sendNumeric(ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return;
	}
	if (!channel->hasMember(_fd))
	{
		sendNumeric(ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return;
	}
	if (cmd.params.size() < 2)
	{
		if (channel->getTopic().empty())
			sendNumeric(RPL_NOTOPIC, channelName + " :No topic is set");
		else
			sendNumeric(RPL_TOPIC, channelName + " :" + channel->getTopic());
		return;
	}
	if (channel->isTopicRestricted() && !channel->isOperator(_fd))
	{
		sendNumeric(ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}
	channel->setTopic(cmd.params[1]);
	g_server->broadcastToChannel(channelName, ":" + buildPrefix(*this) + " TOPIC " + channelName + " :" + cmd.params[1], -1);
}

void Client::cmdMode(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendNumeric(ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return;
	}
	const std::string &channelName = cmd.params[0];
	if (channelName.empty() || channelName[0] != '#')
		return;
	Channel *channel = g_server->getChannel(channelName);
	if (!channel)
	{
		sendNumeric(ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return;
	}
	if (cmd.params.size() < 2)
	{
		std::string modes = "+";
		if (channel->isInviteOnly())     modes += "i";
		if (channel->isTopicRestricted()) modes += "t";
		if (!channel->getKey().empty())  modes += "k";
		if (channel->getUserLimit() > 0) modes += "l";
		sendNumeric(RPL_CHANNELMODEIS, channelName + " " + modes);
		return;
	}
	if (!channel->isOperator(_fd))
	{
		sendNumeric(ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}
	const std::string &modeStr = cmd.params[1];
	bool adding = true;
	size_t paramIdx = 2;
	std::string appliedModes;
	std::string appliedParams;
	char currentSign = '+';

	for (size_t i = 0; i < modeStr.size(); ++i)
	{
		char c = modeStr[i];
		if (c == '+') { adding = true; currentSign = '+'; continue; }
		if (c == '-') { adding = false; currentSign = '-'; continue; }

		if (c == 'i')
		{
			channel->setInviteOnly(adding);
			appliedModes += currentSign;
			appliedModes += c;
		}
		else if (c == 't')
		{
			channel->setTopicRestricted(adding);
			appliedModes += currentSign;
			appliedModes += c;
		}
		else if (c == 'k')
		{
			if (adding)
			{
				if (paramIdx >= cmd.params.size()) continue;
				const std::string &key = cmd.params[paramIdx++];
				channel->setKey(key);
				appliedParams += " " + key;
			}
			else
				channel->setKey("");
			appliedModes += currentSign;
			appliedModes += c;
		}
		else if (c == 'o')
		{
			if (paramIdx >= cmd.params.size()) continue;
			const Client *target = g_server->getClientByNick(cmd.params[paramIdx++]);
			if (!target || !channel->hasMember(target->getFd())) continue;
			if (adding)
				channel->addOperator(target->getFd());
			else
				channel->removeOperator(target->getFd());
			appliedModes += currentSign;
			appliedModes += c;
			appliedParams += " " + target->getNickname();
		}
		else if (c == 'l')
		{
			if (adding)
			{
				if (paramIdx >= cmd.params.size()) continue;
				std::istringstream ss(cmd.params[paramIdx++]);
				size_t limit = 0;
				ss >> limit;
				if (limit == 0) continue;
				channel->setUserLimit(limit);
				std::ostringstream out;
				out << limit;
				appliedParams += " " + out.str();
			}
			else
				channel->setUserLimit(0);
			appliedModes += currentSign;
			appliedModes += c;
		}
		else
			sendNumeric(ERR_UNKNOWNMODE, std::string(1, c) + " :is unknown mode char to me");
	}
	if (!appliedModes.empty())
	{
		g_server->broadcastToChannel(channelName,
			":" + buildPrefix(*this) + " MODE " + channelName + " " + appliedModes + appliedParams, -1);
	}
}
