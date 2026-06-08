/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 02:43:38 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 20:25:46 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <master.h>
#include <Server.h>

Server::Server()
{
	std::vector<std::string> cmd;

	Server::sig = true;
	validCmds.push_back("PASS");
	validCmds.push_back("NICK");
	validCmds.push_back("USER");
	validCmds.push_back("OPER");
	validCmds.push_back("MODE");
	validCmds.push_back("SERVICE");
	validCmds.push_back("QUIT");
	validCmds.push_back("SQUIT");
	validCmds.push_back("JOIN");
	validCmds.push_back("PART");
	validCmds.push_back("MODE");
	validCmds.push_back("TOPIC");
	validCmds.push_back("NAMES");
	validCmds.push_back("LIST");
	validCmds.push_back("INVITE");
	validCmds.push_back("KICK");
	validCmds.push_back("PRIVMSG");
	validCmds.push_back("NOTICE");
	validCmds.push_back("MOTD");
	validCmds.push_back("LUSERS");
	validCmds.push_back("VERSION");
	validCmds.push_back("STATS");
	validCmds.push_back("LINKS");
	validCmds.push_back("TIME");
	validCmds.push_back("CONNECT");
	validCmds.push_back("TRACE");
	validCmds.push_back("ADMIN");
	validCmds.push_back("INFO");
	validCmds.push_back("SERVLIST");
	validCmds.push_back("SQUERY");
	validCmds.push_back("WHO");
	validCmds.push_back("WHOIS");
	validCmds.push_back("WHOWAS");
	validCmds.push_back("KILL");
	validCmds.push_back("PING");
	validCmds.push_back("PONG");
	validCmds.push_back("ERROR");
	validCmds.push_back("AWAY");
	validCmds.push_back("REHASH");
	validCmds.push_back("DIE");
	validCmds.push_back("RESTART");
	validCmds.push_back("SUMMON");
	validCmds.push_back("USERS");
	validCmds.push_back("WALLOPS");
	validCmds.push_back("USERHOST");
	validCmds.push_back("ISON");
}

void		Server::serverThread()
{
	while (Server::sig)
	{
		//TODO
		// Implement poll()
		if (!fds.empty())
			poll(&fds[0], fds.size(), 1000);
	}
	std::cout << YEL << "shutting down" << WHI << '\n';
	closeFds();
	clearClients();
}

void		Server::serverInit(int port)
{
	if (port < 0 || port > PORT_MAX)
		throw std::runtime_error("Port number out of bounds");
	this->port = port;
	sockIt();
	std::cout << GRE << serverSocket << "> Connection succesfull" << WHI << '\n';
	serverThread();
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

void		Server::newClient(const Client &cli)
{
	clients.push_back(cli);
}

void		Server::closeFds()
{
	std::cout << YEL << "---closing all file descriptors---" << WHI << '\n';
	while (!fds.empty())
	{
		close(fds.back().fd);
		fds.pop_back();
	}
}

void		Server::clearClients() { clients.clear(); }

void		Server::handleSig(int signum)
{
	(void)signum;
	Server::sig = false;
}
