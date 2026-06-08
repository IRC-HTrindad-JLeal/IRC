/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 12:15:35 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 12:33:32 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <master.h>
#include <Client.h>

Client::Client() {}

int		Client::getFd() const			{ return fd; }
std::string	Client::getIp() const			{ return ip; }
void		Client::setFd(int FD)			{ fd = FD; }
void		Client::setIp(const std::string &IP)	{ ip = IP; }
