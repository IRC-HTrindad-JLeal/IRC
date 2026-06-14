/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 21:22:51 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/14 17:41:36 by jordanleal       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>

class Client
{
	private:
		int			fd;
		std::string	ip;
		bool		op;

		std::string				readBuffer;
		std::deque<std::string>	sendQueue;
		
		std::string	nickname;
		std::string	username;
		std::string	realname;

		bool	passAccepted;
		bool	hasNickname;
		bool	hasUsername;
		bool	registered;
	
	public:
		Client();
		~Client();
		//Client(bool op);

		int					getFd() const;
		const std::string	&getIp() const;
		bool				getOp() const;

		const std::string	&getNickname() const;
		const std::string	&getUsername() const;
		const std::string	&getRealName() const;

		bool	isPassAccepted() const;
		bool	isRegistered() const;

		void	setFd(int fd);
		void	setIp(const std::string &ip);
		void	setOp(bool op);
		
		void	setNick(const std::string &nickname);
		void	setUsername(const std::string &username);
		void	setRealname(const std::string &realname);
		void	setPassAccepted(bool value);

		bool		appendToReadBuffer(const std::string &data);
		bool		hasCompleteLine() const;
		std::string	popLine();
	
		void		queueMessage(const std::string &message);
		bool		hasPendingOutput() const;
		std::string	&frontOutput();
		void		popOutput();

		void	updateRegistration();
};

