/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 02:43:38 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/07 06:07:41 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <master.h>
#include <Server.h>

Server::Server() { sig = false; }

void		Server::serverThread()
{
	//TODO
	// Implement poll()
}

void		Server::serverInit(int port)
{
	this->port = port;
	sockIt();
	std::cout << GRE << serverSocket << "> Connection succesfull" << WHI << '\n';
	serverThread();
	closeFds();
}

void		Server::sockIt()
{
	struct sockaddr_in	addr;
	struct pollfd		pfd;
	int			magic;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; // setting the socket address
	serverSocket = socket(AF_INET, SOCK_STREAM, 0); // getting the socket fd;
	if (serverSocket < 0)
		throw std::runtime_error("Failed to create the socket");
	magic = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &magic, sizeof(magic)) < 0) // setting the server socket's option to (SO_REUSEADDR) for the purpose of reusing the addres
		throw std::runtime_error("Failed to set the socket with option \"SOL_SOCKET\"");
	if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) < 0) // setting the server socket's option to (O_NONBLOCK) so that the accept function only returns an fd that is available.
		throw std::runtime_error("Failed to set the socket with the option \"O_NONBLOCK\"");
	if (::bind(serverSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) // binding the socket to the address
		throw std::runtime_error("Failed to bind the socket to address");
	if (listen(serverSocket, SOMAXCONN) < 0) // open the socket for the next connections
		throw std::runtime_error("Failed to open the socket for incoming connections");
	pfd.fd = serverSocket;
	pfd.events = POLLIN; // set the poll fd for when there is data to read
	pfd.revents = 0;
	fds.push_back(pfd); // off you go
}
