/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jordanleal <jleal@student.42lisboa.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/14 22:13:03 by jordanleal        #+#    #+#             */
/*   Updated: 2026/06/14 23:20:32 by jordanleal       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Channel.h>
#include <master.h>

Channel::Channel()
	:	name(""),
		topic(""),
		key(""),
		userLimit(0),
		inviteOnly(false),
		topicOnly(true)
{}

Channel::Channel(const std::string &name)
	:	name(name),
		topic(""),
		key(""),
		userLimit(0),
		inviteOnly(false),
		topicOnly(true)
{}

Channel::Channel(const Channel &other)
	:	name(other.name),
		topic(other.topic),
		key(other.key),
		userLimit(other.userLimit),
		inviteOnly(other.inviteOnly),
		topicOnly(other.topicOnly),
		members(other.members),
		invited(other.invited)
{}

Channel &Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		name = other.name;
		topic = other.topic;
		key = other.key;
		userLimit = other.userLimit;
		inviteOnly = other.inviteOnly;
		topicOnly = other.topicOnly;
		members = other.members;
		invited = other.invited;
	}
	return (*this);
}

Channel::~Channel() {}

const std::string	&Channel::getName() const	{ return (name); }
const std::string	&Channel::getTopic() const	{ return (topic); }
const std::string	&Channel::getKey() const	{ return (key); }

size_t	Channel::getUserLimit() const	{ return (userLimit); }
bool	Channel::isInviteOnly() const	{ return (inviteOnly); }
bool	Channel::isTopicOnly() const	{ return (topicOnly); }
bool	Channel::hasKey() const			{ return (!(key == "")); }

void	Channel::setName(const std::string &value) { name = value; }
void	Channel::setTopic(const std::string &value) { topic = value; }
void	Channel::setKey(const std::string &value) { key = value; }
void	Channel::clearKey() { key.clear(); }
void	Channel::setUserLimit(size_t limit) { userLimit = limit; }
void	Channel::clearUserLimit() { userLimit = 0; }
void	Channel::setInviteOnly(bool value) { inviteOnly = value; }
void	Channel::setTopicOnly(bool value) { topicOnly = value; }

bool	Channel::addMember(int fd, bool op)
{
	if (hasMember(fd))
		return (false);
	members[fd] = op;
	return (true);
}

bool	Channel::consumeInvite(int fd)
{
	std::map<int, bool>::iterator it = invited.find(fd);

	if (it == invited.end())
		return (false);
	invited.erase(it);
	return (true);
}

void	Channel::removeMember(int fd)
{
	members.erase(fd);
	invited.erase(fd);
}

bool	Channel::hasMember(int fd) const
{
	return (members.find(fd) != members.end());
}

bool	Channel::isOperator(int fd) const
{
	std::map<int, bool>::const_iterator it = members.find(fd);

	if (it == members.end())
			return (false);
	return (it->second);
}

void	Channel::setOperator(int fd, bool value)
{
	if (hasMember(fd))
		members[fd] = value;
}
		
void	Channel::invite(int fd) { invited[fd] = true; }
void	Channel::uninvite(int fd) { invited.erase(fd); }

bool	Channel::isInvited(int fd) const
{
	return (invited.find(fd) != invited.end());
}


bool	Channel::isFull() const
{
	return (userLimit != 0 && members.size() >= userLimit);
}

bool	Channel::empty() const	{ return (members.empty()); }
size_t	Channel::size() const	{ return (members.size()); }

const std::map<int, bool>	&Channel::getMembers() const
{
	return (members);
}

