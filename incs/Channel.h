/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jordanleal <jleal@student.42lisboa.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/14 22:09:29 by jordanleal        #+#    #+#             */
/*   Updated: 2026/06/14 22:35:21 by jordanleal       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <master.h>

class Channel
{
	private:
		std::string			name;
		std::string			topic;
		std::string			key;
		size_t				userLimit;
		bool				inviteOnly;
		bool				topicOnly;
		std::map<int, bool>	members;
		std::map<int, bool>	invited;
	
	public:
		Channel();
		Channel(const std::string &name);
		Channel(const Channel &other);
		Channel &operator=(const Channel &other);
		~Channel();

		const std::string	&getName() const;
		const std::string	&getTopic() const;
		const std::string	&getKey() const;

		size_t	getUserLimit() const;
		bool	isInviteOnly() const;
		bool	isTopicOnly() const;

		void	setName(const std::string &name);
		void	setTopic(const std::string &topic);

		void	setKey(const std::string &key);
		void	clearKey();

		void	setUserLimit(size_t limit);
		void	clearUserLimit();

		void	setInviteOnly(bool value);
		void	setTopicOnly(bool value);

		bool	addMember(int fd, bool op);
		void	removeMember(int fd);
		bool	hasMember(int fd) const;
		bool	isOperator(int fd) const;
		void	setOperator(int fd, bool value);
		
		void	invite(int fd);
		void	uninvite(int fd);
		bool	isInvited(int fd) const;

		bool	isFull() const;
		bool	empty() const;
		size_t	size() const;

		const std::map<int, bool>	&getMembers() const;
};
