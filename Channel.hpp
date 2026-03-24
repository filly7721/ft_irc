#pragma once

#include <set>
#include <string>

class Channel
{
private:
	Channel();

private:
	std::string _name;
	std::string _topic;
	std::string _key;
	std::set<int> _members;
	std::set<int> _operators;
	std::set<int> _invited;
	bool _inviteOnly;
	size_t _userLimit;

public:
	Channel(const std::string &name);
	Channel(const Channel &copy);
	~Channel();
	const Channel &operator=(const Channel &copy);

	const std::string &getName() const;
	const std::string &getTopic() const;
	const std::string &getKey() const;
	size_t getUserLimit() const;
	bool isInviteOnly() const;
	bool isFull() const;
	bool hasMember(int fd) const;
	bool isOperator(int fd) const;
	bool isInvited(int fd) const;
	size_t memberCount() const;
	const std::set<int> &getMembers() const;

	void setTopic(const std::string &topic);
	void setKey(const std::string &key);
	void setUserLimit(size_t userLimit);
	void setInviteOnly(bool inviteOnly);

	void addMember(int fd);
	void removeMember(int fd);
	void addOperator(int fd);
	void removeOperator(int fd);
	void addInvite(int fd);
	void removeInvite(int fd);
};
