/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 21:22:51 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 21:18:37 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>

class Client
{
	private:
		int		fd;
		std::string	ip;
		bool		op;
	public:
		Client();
		Client(bool op);
		int		getFd() const;
		std::string	getIp() const;
		bool		getOp() const;
		void		setFd(int FD);
		void		setIp(const std::string &IP);
		void		setOp(bool op);
};
