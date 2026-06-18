/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 21:17:45 by mely-pan          #+#    #+#             */
/*   Updated: 2026/06/18 22:47:55 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <master.h>
#include <Server.h>
#include <Client.h>
#include <Message.h>

class CommandHandler
{
	private:
		typedef bool (CommandHandler::*CommandFt)(Server &, Client &, const Message &);
		std::map<std::string, CommandFt>	_handlers;

		bool	dispatch(const std::string &cmd, Server&Server, Client &client, const Message &msg);
		bool	requiresAuth(const std::string &cmd) const;
		
		void	inItHandlers();

		//Commmands
		bool	pass(Server &server, Client &client, const Message &msg);	
		bool	nick(Server &server, Client &client, const Message &msg);	
		bool	user(Server &server, Client &client, const Message &msg);	
		bool	cap(Server &server, Client &client, const Message &msg);	
		bool	ping(Server &server, Client &client, const Message &msg);	
		bool	pong(Server &server, Client &client, const Message &msg);	
		bool	quit(Server &server, Client &client, const Message &msg);	
		bool	join(Server &server, Client &client, const Message &msg);	
		bool	privmsg(Server &server, Client &client, const Message &msg);	
		bool	mode(Server &server, Client &client, const Message &msg);	
		bool	topic(Server &server, Client &client, const Message &msg);	
		bool	invite(Server &server, Client &client, const Message &msg);	
		bool	kick(Server &server, Client &client, const Message &msg);	
	public:
		CommandHandler();
		~CommandHandler();
		bool	execute(Server &server, Client &client, const Message &msg);
};
