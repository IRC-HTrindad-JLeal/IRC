/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 22:53:42 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/14 23:05:56 by jordanleal       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <master.h>
#include <Client.h>
#include <Message.h>
#include <Channel.h>

class Server
{
	private:
		int							port;
		int							serverSocket;
		static bool					sig;

		std::map<int, Client>			clients;
		std::map<std::string, int>		nicknames;
		std::map<std::string, Channel>	channels;

		std::vector<struct pollfd>	fds;
		std::vector<std::string>	validCmds;
		std::string					password;

		void	disconnectClient(int fd);
		bool	flushClientOutput(int fd);
		void	setPollOut(int fd, bool enabled);
		void	dispatchMessage(Client &client, const Message &msg);

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
		void				sendToClient(Client &client, const std::string &reply);

		Client	*findClientByFd(int fd);
		Client	*findClientByNickname(const std::string &nickname);

		bool	isNicknameAvailable(const std::string &nick) const;
		bool	registerNickname(Client &client, const std::string &newNickname);

		Channel	*findChannel(const std::string &name);
		Channel	&getOrCreateChannel(const std::string &name);
		void	removeClientFromChannels(int fd);
};
