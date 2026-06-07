/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 21:22:51 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/06 22:41:11 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "master.h"

class Client
{
	private:
		int		fd;
		std::string	ip;
	public:
		Client();
		int		getFd() const;
		std::string	getIp() const;
		void		setFd(int _fd);
		void		setIp(const std::string &_ip);
};
