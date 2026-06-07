/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 21:22:51 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/07 05:02:01 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>

class Client
{
	private:
		int		fd;
		std::string	ip;
	public:
		Client();
		int		getFd() const;
		std::string	getIp() const;
		void		setFd(int FD);
		void		setIp(const std::string &IP);
};
