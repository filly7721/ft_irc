#include "Channel.hpp"

Channel::Channel(const std::string &name)
	: _name(name), _inviteOnly(false), _topicRestricted(false), _userLimit(0)
{
}

Channel::Channel(const Channel &copy)
{
	*this = copy;
}

Channel::~Channel()
{
}

const Channel &Channel::operator=(const Channel &copy)
{
	if (this == &copy)
		return *this;
	_name = copy._name;
	_topic = copy._topic;
	_key = copy._key;
	_members = copy._members;
	_operators = copy._operators;
	_invited = copy._invited;
	_inviteOnly = copy._inviteOnly;
	_topicRestricted = copy._topicRestricted;
	_userLimit = copy._userLimit;
	return *this;
}

const std::string &Channel::getName() const
{
	return _name;
}

const std::string &Channel::getTopic() const
{
	return _topic;
}

const std::string &Channel::getKey() const
{
	return _key;
}

size_t Channel::getUserLimit() const
{
	return _userLimit;
}

bool Channel::isInviteOnly() const
{
	return _inviteOnly;
}

bool Channel::isFull() const
{
	return (_userLimit != 0 && _members.size() >= _userLimit);
}

bool Channel::hasMember(int fd) const
{
	return _members.find(fd) != _members.end();
}

bool Channel::isOperator(int fd) const
{
	return _operators.find(fd) != _operators.end();
}

bool Channel::isInvited(int fd) const
{
	return _invited.find(fd) != _invited.end();
}

size_t Channel::memberCount() const
{
	return _members.size();
}

const std::set<int> &Channel::getMembers() const
{
	return _members;
}

void Channel::setTopic(const std::string &topic)
{
	_topic = topic;
}

void Channel::setKey(const std::string &key)
{
	_key = key;
}

void Channel::setUserLimit(size_t userLimit)
{
	_userLimit = userLimit;
}

void Channel::setInviteOnly(bool inviteOnly)
{
	_inviteOnly = inviteOnly;
}

bool Channel::isTopicRestricted() const
{
	return _topicRestricted;
}

void Channel::setTopicRestricted(bool value)
{
	_topicRestricted = value;
}

void Channel::addMember(int fd)
{
	_members.insert(fd);
}

void Channel::removeMember(int fd)
{
	_members.erase(fd);
	_operators.erase(fd);
	_invited.erase(fd);
}

void Channel::addOperator(int fd)
{
	_operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
	_operators.erase(fd);
}

void Channel::addInvite(int fd)
{
	_invited.insert(fd);
}

void Channel::removeInvite(int fd)
{
	_invited.erase(fd);
}
