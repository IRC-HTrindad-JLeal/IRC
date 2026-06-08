/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 22:53:42 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 20:23:45 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>
#include <Client.h>

class Server
{
	private:
		int				port;
		int				serverSocket;
		static bool			sig;
		std::vector<Client>		clients;
		std::vector<struct pollfd>	fds;
		std::vector<std::string>	validCmds;
	public:
		Server();
		void		serverThread();
		void		serverInit(int port);
		void		sockIt();
		void		newClient(const Client &cli);
		void		retrieveData(int fd);
		static void	handleSig(int signum);
		void		closeFds();
		void		clearClients();
};
