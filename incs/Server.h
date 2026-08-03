/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 22:53:42 by htrindad          #+#    #+#             */
/*   Updated: 2026/07/08 21:35:16 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <master.h>
#include <Client.h>
#include <Message.h>
#include <Channel.h>

class CommandHandler;

class Server
{
	private:
		static volatile sig_atomic_t sig;

		int							port;
		int							serverSocket;

		std::map<int, Client>			clients;
		std::map<std::string, int>		nicknames; // The nicknames map is just an index of nicknames for quick search, only update nicknames through the registerNickanme() function.
		std::map<std::string, Channel>	channels;

		std::vector<struct pollfd>	fds;
		// std::vector<std::string>	validCmds;
		std::string					password;
		std::string					creationDate;
		CommandHandler				*_commandHandler;

		void	disconnectClient(int fd);
		bool	flushClientOutput(int fd);
		void	setPollOut(int fd, bool enabled);
		bool	dispatchMessage(Client &client, const Message &msg);

	public:
		Server();
		~Server();

		void				serverThread();
		void				serverInit(int port, const std::string &password);
		void				sockIt();
		void				newClient(const Client &cli);
		void				acceptClient();
		bool				retrieveData(int fd);
		static void			handleSig(int signum);
		void				closeFds();
		void				clearClients();
		const std::string	&getPassword() const;
		const std::string	&getCreationDate() const;
		bool				sendToClient(Client &client, const std::string &reply);

		Client	*findClientByFd(int fd);
		Client	*findClientByNickname(const std::string &nickname);

		bool	isNicknameAvailable(const std::string &nick) const;
		bool	registerNickname(Client &client, const std::string &newNickname);

		Channel	*findChannel(const std::string &name);
		Channel	&getOrCreateChannel(const std::string &name);
		void	removeClientFromChannels(int fd);

		bool	broadcastToChannel(const Channel &channel, const std::string &message, int exceptFd);
		bool	broadcastToClientChannels(const Client &client, const std::string &message, int exceptFd);
		bool	broadcastAllRegistered(const std::string &message, int exceptFd);
};
