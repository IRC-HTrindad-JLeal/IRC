/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 02:43:38 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/07 02:50:35 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <master.h>

Server::Server() { sig = false; }

void	Server::serverSock()
{
	struct sockaddr_in	addr;
	struct pollfd		pfd;

	addr.sin_family = AF_INET; // setting the addres to ipv4
	addr.sin_port = htons(port); // setting the address port to the byte order using htons
	addr.sin_addr.s_addr = INADDR_ANY;
	serverSocket = socket(AF_INET, SOCK_STREAM);
}
