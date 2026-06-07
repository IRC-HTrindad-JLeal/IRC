/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 22:53:42 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/07 03:26:08 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>

class Server
{
	private:
		int				port;
		int				serverSocket;
		static bool			sig;
		std::vector<Client>		clients;
		std::vector<struct pollfd>	fds;
	public:
		Server();
		void		serverInit();
		void		serverSock();
		void		newClient();
		void		retrieveData(int fd);
		static void	handleSig(int signum);
		void		closeFds();
		void		clearClients();
};
