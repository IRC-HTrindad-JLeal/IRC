/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 02:43:38 by htrindad          #+#    #+#             */
/*   Updated: 2026/07/08 21:43:15 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Server.h>
#include <CommandHandler.h>
#include <Message.h>
#include <master.h>

volatile sig_atomic_t Server::sig = 1;
volatile sig_atomic_t Server::receivedSignal = 0;

static std::string	signalName(int signum)
{
	if (signum == SIGINT)
		return ("Interrupt");
	if (signum == SIGQUIT)
		return ("Quit");
	return ("Unknown");
}

void	Server::logStatus(int level, const std::string &message) const
{
		std::cout << "[" << getpid() << ":" << level << " "
			<< (std::time(NULL) - startTime) << "] "
			<< message << '\n';
}

static std::string	clientHostPort(const Client &client)
{
	std::stringstream ss;

	ss << client.getIp() << ":" << client.getPort();
	return (ss.str());
}

static std::string	clientMask(const Client &client)
{
	std::stringstream ss;

	ss << client.getNickname() << "!"
			<< client.getUsername() << "@"
			<< client.getIp();
	return (ss.str());
}

Server::Server()
{
	_commandHandler = NULL;
	serverSocket = -1;
}

Server::~Server()
{
	delete _commandHandler;
	//validCmds.clear();
	clearClients();
	closeFds();
}

const std::string	&Server::getPassword() const { return this->password; }

const std::string	&Server::getCreationDate() const { return this->creationDate; }

void		Server::serverThread()
{
	while (Server::sig != 0)
	{
		if (fds.empty())
			break;

		int ready = poll(&fds[0], fds.size(), 1000);

		if (ready < 0)
		{
			int	err = errno;
			if (err == EINTR)
				continue;
			throw std::runtime_error(std::string("Poll() failed: ") + strerror(err));
		}
		if (ready == 0)
		{
			// Timeout. Noting happened.
			continue;
		}

		size_t				nfds = fds.size();

		for (size_t i = nfds; i > 0; --i)
		{
			struct pollfd pfd = fds[i - 1];
			int fd = pfd.fd;
			bool disconnect = false;
			
			std::string	reason;

			if (pfd.revents & POLLNVAL) 
			{
				if (fd != serverSocket)
				{
					reason = "Invalid socket descriptor";
					disconnect = true;
				}
			}
			else if (pfd.revents & POLLERR)
			{
				if (fd != serverSocket)
				{
					reason = "Socket error";
					disconnect = true;
				}
			}
			else if (pfd.revents & POLLHUP)
			{
				if (fd != serverSocket)
				{
					reason = "Client closed connection";
					disconnect = true;
				}
			}
			else if (fd == serverSocket && (pfd.revents & POLLIN))
				acceptClient();
			else
			{
				if ((pfd.revents & POLLIN) && !retrieveData(fd, reason))
					disconnect = true;
				if (!disconnect && (pfd.revents & POLLOUT) && !flushClientOutput(fd, reason))
					disconnect = true;
			}
			if (disconnect)
			{
				if (reason.empty())
					reason = "Client closed connection";
				disconnectClient(fd, reason);
			}
		}
	}

	if (receivedSignal != 0)
	{
		std::stringstream signalMsg;

		signalMsg << "Got signal \"" << signalName(receivedSignal) << "\" ...";
		logStatus(6, signalMsg.str());
	}

	logStatus(5, "Server going down NOW!");

	for (size_t i = fds.size(); i > 0; --i)
	{
		int fd = fds[i - 1].fd;
		
		if (fd != serverSocket)
			disconnectClient(fd, "Server shutdown");
	}

	int listenerCount = 0;

	for (size_t i = 0; i < fds.size(); ++i)
	{
		if (fds[i].fd == serverSocket)
			++listenerCount;
	}

	std::stringstream socketMsg;
	socketMsg << "Shutting down all listening sockets ("
		<< listenerCount << " total) ...";
	logStatus(6, socketMsg.str());

	closeFds();
	clearClients();

	std::stringstream doneMsg;
	doneMsg << "ft_irc done, served " << acceptedConnectionCount
		<< " connection";
	if (acceptedConnectionCount != 1)
		doneMsg << 's';
	doneMsg << '.';
	logStatus(5, doneMsg.str());
}

bool	Server::flushClientOutput(int fd, std::string &reason)
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
			int err = errno;

			if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK)
				return (true);
			reason = std::string("Write error: ") + strerror(err);
			return (false);
		}

		if (sent == 0)
		{
			reason = "Write error: send returned 0";
			return (false);
		}

		client->consumeOutput(static_cast<size_t>(sent));

		if (client->hasPendingOutput())
			return(true);
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
	while (true)
	{
		struct sockaddr_in	clientAddr;
		socklen_t			len = sizeof(clientAddr);
		bool				clientAdded = false;
	
		int clientFd = accept(serverSocket, (struct sockaddr *)&clientAddr, &len);
		if (clientFd < 0)
		{
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			std::cerr << RED << "accept() failed: " << strerror(errno) << WHI << '\n';
			break;
		}
	
		if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
		{
			std::cerr << RED << clientFd << "> failed to set O_NONBLOCK: " << strerror(errno) << WHI << '\n';
			close(clientFd);
			continue;
		}
	
		try
		{
			Client cli;
			cli.setFd(clientFd);
			cli.setIp(inet_ntoa(clientAddr.sin_addr));
			cli.setPort(ntohs(clientAddr.sin_port));
			newClient(cli);
			clientAdded = true;
	
			struct pollfd pfd;
			pfd.fd = clientFd;
			pfd.events = POLLIN;
			pfd.revents = 0;
			fds.push_back(pfd);
			++acceptedConnectionCount;

			std::stringstream msg;
			msg << "Accepted connection " << clientFd
				<< " from \"" << cli.getIp() << ":" << cli.getPort()
				<< "\" on socket " << serverSocket << ".";
			logStatus(6, msg.str());

			//std::cout << GRE << clientFd << "> client connected" << WHI << '\n';
			
		}
		catch (const std::exception &e)
		{
			if (clientAdded)
				clients.erase(clientFd);
			close(clientFd);
			std::cerr << RED << clientFd << "> failed to add client: " << e.what() << WHI << '\n';
		}
	}
}

void		Server::serverInit(int port, const std::string &password)
{
	if (port <= 0 || port > PORT_MAX)
		throw std::runtime_error("Port number out of bounds");
	if (password.empty())
		throw std::runtime_error("Password cannot be empty");
	this->port = port;
	this->password = password;
	startTime = std::time(NULL);
	acceptedConnectionCount = 0;

	char buf[64];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&startTime));
	creationDate = std::string(buf);

	logStatus(5, "ft_irc 42 starting ...");

	std::stringstream config;
	config << "Using command-line configuration: port " << port << ".";
	logStatus(6, config.str());

	_commandHandler = new CommandHandler();
	sockIt();

	logStatus(6, "IO subsystem: poll (initial descriptors 1).");
	logStatus(5, "Server \"ft_irc\" ready.");

	serverThread();
}

bool Server::retrieveData(int fd, std::string &reason)
{
	Client	*client = findClientByFd(fd);
	char	buffer[LINE_LEN_BUF_MAX];
	ssize_t bytes;

	if (!client)
		return (true);

	bytes = recv(fd, buffer, LINE_LEN_BUF_MAX, 0);
	if (bytes == 0)
	{
		reason = "Client closed connection";
		return (false);
	}
	if (bytes < 0)
	{
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		reason = std::string("Read error: ") + strerror(errno);
		return (false);
	}

	try
	{
		std::string chunk(buffer, static_cast<size_t>(bytes));

		if (!client->appendToReadBuffer(chunk))
		{
			reason = "Input line too long";
			return (false);
		}

		while(client->hasCompleteLine())
		{
			std::string line = client->popLine();
	
			if (line.empty())
				continue;

			Message msg;
			try
			{
				msg = Message::parse(line);
			}
			catch (const std::exception &e)
			{
				//TODO: queue IRC error response instead of just logging
				std::cerr << RED << fd << "> parse error: " << e.what() << WHI << '\n';
				continue;
			}
			if (!dispatchMessage(*client, msg))
			{
				if (msg.getCommand() == "QUIT")
					reason = "Got QUIT command";
				else
					reason = "Client closed connection";
				return (false);
			}
		}
	}
	catch (const std::bad_alloc &e)
	{
		reason = std::string("Resource error: ") + e.what();
		std::cerr << RED << fd << "> resource error while processing input" << WHI << '\n';
		return (false);
	}
	catch (const std::exception &e)
	{
		reason = std::string("Input processing error: ") + e.what();
		std::cerr << RED << fd << "> input processing error: " << e.what() << WHI << '\n';
		return (false);
	}
	catch (...)
	{
		reason = "Unknown input processing error";
		std::cerr << RED << fd << "> unknown input processing error" << WHI << '\n';
		return (false);
	}
	return (true);
}

bool		Server::dispatchMessage(Client &client, const Message &msg)
{
	return (this->_commandHandler->execute(*this, client, msg));
}

bool		Server::sendToClient(Client &client, const std::string &reply)
{
	std::string message = reply;

	if (message.size() < 2 || message.substr(message.size() - 2) != "\r\n")
		message += "\r\n";

	if ((!client.queueMessage(message)))
		return (false);

	setPollOut(client.getFd(), true);
	return (true);
}

void		Server::sockIt()
{
	struct sockaddr_in	addr;
	struct pollfd		pfd;
	int					enable;
	int					fd;

	std::memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; // setting the socket address

	fd = socket(AF_INET, SOCK_STREAM, 0); // getting the socket fd;
	
	if (fd < 0)
		throw std::runtime_error("Failed to create the socket");

	try
	{
		enable = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) // setting the server socket's option to (SO_REUSEADDR) for the purpose of reusing the addres
			throw std::runtime_error("Failed to set the socket with option \"SOL_SOCKET\"");
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) // setting the server socket's option to (O_NONBLOCK) so that the accept function only returns an fd that is available.
			throw std::runtime_error("Failed to set the socket with the option \"O_NONBLOCK\"");
		if (::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) // binding the socket to the address
			throw std::runtime_error("Failed to bind the socket to address");
		if (listen(fd, SOMAXCONN) < 0) // open the socket for the next connections
			throw std::runtime_error("Failed to open the socket for incoming connections");
		pfd.fd = fd;
		pfd.events = POLLIN; // set the poll fd for when there is data to read
		pfd.revents = 0;
		fds.push_back(pfd); // off you go

		serverSocket = fd;
		std::stringstream msg;
		msg << "Now listening on [0.0.0.0]:" << port
			<< " (socket " << serverSocket << ").";
		logStatus(6, msg.str());
	}
	catch (...)
	{
		close(fd);
		serverSocket = -1;
		throw;
	}
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
	// std::cout << YEL << "---closing all file descriptors---" << WHI << '\n';
	while (!fds.empty())
	{
		close(fds.back().fd);
		fds.pop_back();
	}
}

void		Server::disconnectClient(int fd, const std::string &reason)
{
	Client		*client = findClientByFd(fd);
	std::string	hostPort = "unknown:0";
	std::string	mask;
	bool		registered = false;

	if (fd == serverSocket)
		return;
	
	if (client)
	{
		hostPort = clientHostPort(*client);
		registered = client->isRegistered();
		if (registered)
			mask = clientMask(*client);
	}

	std::stringstream shutting;
	shutting << "Shutting down connection " << fd
		<< " (" << reason << ") with \""
		<< hostPort << "\" ...";
	logStatus(6, shutting.str());

	if (registered)
	{
		std::stringstream msg;
		msg << "User \"" << mask << "\" unregistered "
			<< "(connection " << fd << "): " << reason << ":";
		logStatus(5, msg.str());
	}
	else
	{
		std::stringstream msg;
		msg << "Client unregistered (connection " << fd
			<< "): " << reason << ".";
		logStatus(5, msg.str());
	}

	if (client && !client->getNickname().empty())
	{
		std::map<std::string, int>::iterator it;
		it = nicknames.find(client->getNickname());
		if (it != nicknames.end() && it->second == fd)
			nicknames.erase(it);
	}

	removeClientFromChannels(fd);
	close(fd);

	std::vector<struct pollfd>::iterator fdit;

	for (fdit = fds.begin(); fdit != fds.end(); ++fdit)
	{
		if (fdit->fd == fd)
		{
			fds.erase(fdit);
			break;
		}
	}
	
	clients.erase(fd);

	std::stringstream closed;
	closed << "Connection " << fd << " with \""
		<< hostPort << "\" closed.";
	logStatus(6, closed.str());
}

void		Server::clearClients() {
		clients.clear();
		nicknames.clear();
		channels.clear();
}

void		Server::handleSig(int signum)
{
	Server::receivedSignal = signum;
	Server::sig = 0;
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

bool	Server::broadcastToChannel(const Channel &channel, const std::string &message, int exceptFd)
{
	const std::map<int, bool>			&members = channel.getMembers();
	std::map<int, bool>::const_iterator	it;
	bool								ok = true;

	for (it = members.begin(); it != members.end(); ++it)
	{
		if (it->first == exceptFd)
			continue;
		Client *client = findClientByFd(it->first);
		if (!client)
			continue;
		if (!sendToClient(*client, message))
			ok = false;
	}
	return ok;
}

bool	Server::broadcastToClientChannels(const Client &client, const std::string &message, int exceptFd)
{
	std::set<int>	recipients;
	bool			ok = true;
	int				cFd = client.getFd();

	for(std::map<std::string, Channel>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (!it->second.hasMember(cFd))
			continue;
		const std::map<int, bool> &members = it->second.getMembers();
		for (std::map<int, bool>::const_iterator m = members.begin(); m != members.end(); ++m)
		{
			if (m->first != exceptFd)
				recipients.insert(m->first);
		}
	}

	for (std::set<int>::iterator it = recipients.begin(); it != recipients.end(); ++it)
	{
		Client *target = findClientByFd(*it);
		if (!target || !sendToClient(*target, message))
			ok = false;
	}
	return (ok);
}

bool	Server::broadcastAllRegistered(const std::string &message, int exceptFd) 
{
	bool ok = true;

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second.isRegistered() && it->first != exceptFd)
			if (!sendToClient(it->second, message))
				ok = false;
	}
	return ok;
}

bool	Server::isNicknameAvailable(const std::string &nick) const
{
	return (nicknames.find(nick) == nicknames.end());
}

bool	Server::registerNickname(Client &client, const std::string &newNickname)
{
	std::string	oldNickname = client.getNickname();
	int			fd = client.getFd();

	if (newNickname.empty())
		return (false);
	if (newNickname == oldNickname)
		return (true);
	if (!isNicknameAvailable(newNickname))
		return (false);

	if (!oldNickname.empty())
	{
		std::map<std::string, int>::iterator it = nicknames.find(oldNickname);

		if (it != nicknames.end() && it->second == fd)
			nicknames.erase(it);
	}

	client.setNick(newNickname);
	nicknames[newNickname] = fd;
	
	return (true);
}

Client	*Server::findClientByNickname(const std::string &nickname)
{
	std::map<std::string, int>::iterator it = nicknames.find(nickname);

	if (it == nicknames.end())
		return NULL;
	return (findClientByFd(it->second));
}

