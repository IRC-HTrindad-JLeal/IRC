/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 12:15:35 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 20:27:33 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Client.h>

Client::Client() : fd(-1), ip() {}

int		Client::getFd() const			{ return fd; }
std::string	Client::getIp() const			{ return ip; }
void		Client::setFd(int FD)			{ fd = FD; }
void		Client::setIp(const std::string &IP)	{ ip = IP; }
