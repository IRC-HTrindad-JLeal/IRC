/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 02:43:38 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/14 23:18:07 by jordanleal       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Server.h>
#include <Message.h>
#include <master.h>

bool	Server::sig = true;

Server::Server()
{
	validCmds.push_back("PASS");
	validCmds.push_back("QUIT");
	validCmds.push_back("PING");
	validCmds.push_back("PONG");
	validCmds.push_back("KICK");
	validCmds.push_back("INVITE");
	validCmds.push_back("TOPIC");
	validCmds.push_back("MODE");
	validCmds.push_back("PRIVMSG");
	validCmds.push_back("JOIN");
	validCmds.push_back("USER");
	validCmds.push_back("NICK");
	validCmds.push_back("CAP");
}

Server::~Server()
{
	validCmds.clear();
	clearClients();
	closeFds();
}

const std::string	&Server::getPassword() const { return this->password; }

void		Server::serverThread()
{
	while (Server::sig)
	{
		if (fds.empty())
			break;

		int ready = poll(&fds[0], fds.size(), 1000);

		if (ready < 0)
		{
			// If interupted by signal the function needs to continue so Server::sig can stop the loop.
			continue;
		}
		if (ready == 0)
		{
			// Timeout. Noting happened.
			continue;
		}

		std::vector<int>	toDisconnect;
		size_t				nfds = fds.size();
		for (size_t i = 0; i < nfds; ++i)
		{
			int fd = fds[i].fd;

			if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) 
			{
				if (fd != serverSocket)
					toDisconnect.push_back(fd);
				continue;
			}
			if (fd == serverSocket && (fds[i].revents & POLLIN))
				acceptClient();
			else if (fds[i].revents & POLLIN)
			{
				if (!retrieveData(fd))
				{
					toDisconnect.push_back(fd);
					continue;
				}
			}
			if (fds[i].revents & POLLOUT)
			{
				if (!flushClientOutput(fd))
					toDisconnect.push_back(fd);
			}
		}

		for (size_t i = 0; i < toDisconnect.size(); ++i)
			disconnectClient(toDisconnect[i]);
	}

	std::cout << YEL << "shutting down" << WHI << '\n';
	closeFds();
	clearClients();
}

bool	Server::flushClientOutput(int fd)
{
	Client *client = findClientByFd(fd);

	if (!client)
		return (true);

	while (client->hasPendingOutput())
	{
		std::string &msg = client->frontOutput();

		ssize_t sent = send(fd, msg.c_str(), msg.size(), 0);

		if (sent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return (true);
			return (false);
		}

		if (sent == 0)
			return (false);

		if (static_cast<size_t>(sent) < msg.size())
		{
			msg.erase(0, sent);
			return (true);
		}

		client->popOutput();
	}

	if (!client->hasPendingOutput())
		setPollOut(fd, false);
	return (true);
}

void		Server::setPollOut(int fd, bool enabled)
{
	std::vector<struct pollfd>::iterator it;

	for (it = fds.begin(); it != fds.end(); ++it)
	{
		if (it->fd == fd)
		{
			if (enabled)
				it->events |= POLLOUT;
			else
				it->events &= ~POLLOUT;
			return;
		}
	}
}

void		Server::newClient(const Client &cli)
{
	std::pair<std::map<int, Client>::iterator, bool>	result;

	result = clients.insert(std::make_pair(cli.getFd(), cli));
	if (!result.second)
		throw std::runtime_error("Client fd already exists");
}

void Server::acceptClient()
{
	struct sockaddr_in	clientAddr;
	socklen_t			len = sizeof(clientAddr);

	int clientFd = accept(serverSocket, (struct sockaddr *)&clientAddr, &len);
	if (clientFd < 0)
		return;

	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags < 0)
	{
		close(clientFd);
		return;
	}
	if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(clientFd);
		return;
	}

	try
	{
		Client cli;
		cli.setFd(clientFd);
		cli.setIp(inet_ntoa(clientAddr.sin_addr));
		newClient(cli);
		std::cout << GRE << clientFd << "> client connected" << WHI << '\n';

		struct pollfd pfd;
		pfd.fd = clientFd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		fds.push_back(pfd);
	}
	catch (const std::exception &e)
	{
		close(clientFd);
		std::cerr << RED << clientFd << "> failed to add client: " << e.what() << WHI << '\n';
		return;
	}
}

void		Server::serverInit(int port, const std::string &password)
{
	if (port < 0 || port > PORT_MAX)
		throw std::runtime_error("Port number out of bounds");
	if (password.empty())
		throw std::runtime_error("Password cannot be empty");
	this->port = port;
	this->password = password;
	sockIt();
	std::cout << GRE << serverSocket << "> Connection succesfull" << WHI << '\n';
	serverThread();
}

bool Server::retrieveData(int fd)
{
	Client	*client = findClientByFd(fd);
	char	buffer[LINE_LEN_BUF_MAX];
	ssize_t bytes;

	if (!client)
		return (true);

	bytes = recv(fd, buffer, LINE_LEN_BUF_MAX, 0);
	if (bytes == 0)
	{
		return (false);
	}
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		return (false);
	}

	if (!client->appendToReadBuffer(std::string(buffer, bytes)))
		return (false);

	while(client->hasCompleteLine())
	{
		std::string line = client->popLine();

		if (line.empty())
			continue;

		try
		{
			Message msg = Message::parse(line, validCmds);
			//TODO: dispatch msg to handlers Ex. dispatchMessagge(*client, msg);
		}
		catch (const std::exception &e)
		{
			//TODO: queue IRC error response instead of just logging
			std::cerr << RED << fd << "> parse error: " << e.what() << WHI << '\n';
		}
	}
	return (true);
}

void		Server::dispatchMessage(Client &client, const Message &msg)
{
	(void)client;
	(void)msg;

	//TODO implement command logic
}

void		Server::sendToClient(Client &client, const std::string &reply)
{
	std::string message = reply;

	if (message.size() < 2 || message.substr(message.size() - 2) != "\r\n")
		message += "\r\n";

	client.queueMessage(message);
	setPollOut(client.getFd(), true);
}

void		Server::sockIt()
{
	struct sockaddr_in	addr;
	struct pollfd		pfd;
	int					magic;

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

Client	*Server::findClientByFd(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);

	if (it == clients.end())
		return (NULL);
	return (&it->second);
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

void		Server::disconnectClient(int fd)
{
	std::vector<struct pollfd>::iterator	fdit;

	if (fd == serverSocket)
		return;

	close(fd);

	for (fdit = fds.begin(); fdit != fds.end(); ++fdit)
	{
		if (fdit->fd == fd)
		{
			fds.erase(fdit);
			break;
		}
	}
	
	clients.erase(fd);

	std::cout << YEL << fd << "> client disconnected" << WHI << '\n';
}

void		Server::clearClients() { if (!clients.empty()) clients.clear(); }

void		Server::handleSig(int signum)
{
	(void)signum;
	Server::sig = false;
}

Channel	*Server::findChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = channels.find(name);

	if (it == channels.end())
		return (NULL);
	return (&it->second);
}

Channel	&Server::getOrCreateChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = channels.find(name);

	if (it != channels.end())
		return (it->second);

	return (channels.insert(std::make_pair(name, Channel(name))).first->second);
}

void	Server::removeClientFromChannels(int fd)
{
	std::map<std::string, Channel>::iterator it = channels.begin();

	while (it != channels.end())
	{
		it->second.removeMember(fd);
		if (it->second.empty())
		{
			std::map<std::string, Channel>::iterator toErase = it;
			++it;
			channels.erase(toErase);
		}
		else
			++it;
	}
}

