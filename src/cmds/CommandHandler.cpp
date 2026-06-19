/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:55:39 by mely-pan          #+#    #+#             */
/*   Updated: 2026/06/19 12:13:53 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <../../incs/CommandHandler.h>

void	CommandHandler::CommandHandler()
{
	inItHandlers();
}

void	CommandHandler::inItHandlers()
{
	_handlers["PASS"] = &CommandHandler::pass;
	_handlers["NICK"] = &CommandHandler::nick;
	_handlers["USER"] = &CommandHandler::user;
	_handlers["CAP"] = &CommandHandler::cap;
	_handlers["PING"] = &CommandHandler::ping;
	_handlers["PONG"] = &CommandHandler::pong;
	_handlers["QUIT"] = &CommandHandler::quit;
	_handlers["JOIN"] = &CommandHandler::join;
	_handlers["PRIVMSG"] = &CommandHandler::privmsg;
	_handlers["MODE"] = &CommandHandler::mode;
	_handlers["TOPIC"] = &CommandHandler::topic;
	_handlers["INVITE"] = &CommandHandler::invite;
	_handlers["KICK"] = &CommandHandler::kick;
}

bool	CommandHandler::requiresAuth(const std::string &cmd) const
{
	return (
		cmd != "PASS" &&
		cmd != "NICK" &&
		cmd != "USER" &&
		cmd != "PING" &&
		cmd != "PONG" &&
		cmd != "CAP" &&
		cmd != "QUIT"
	);
}

bool	CommandHandler::execute(Server &server, Client &client, const Message &msg)
{
	const	std::string &cmd = msg.getCommand();

	return dispatch(cmd, server, client, msg);
}

bool	CommandHandler::dispatch(const std::string &cmd, Server &server, Client &client, const Message &msg)
{
	if (requiresAuth(cmd) && !client.isRegistered())
	{
		Server.sendToClient(client, ":server 451 :You have not registered");
		return true;
	}
	std::map<std::string, CommandFt>::iterator it = _handlers.find(cmd);

	if (it == _handlers.end())
	{
		Server.sendToClient(client, ":server 421 :Uknown command");
		return true;
	}
	return (this->*(it->second))(server, client, msg);
}

bool	CommandHandler::pass(Server &server, Client &client, const Message &msg)
{
	if (client.isPassAccepted())
		return true;
	if (msg.getParams().empty())
	{
		server.sendToClient(client, ":server 461 :Not enough parameters");
		return true;
	}
	if (msg.getParams()[0] != server.getPassword())
	{
		server.sendToClient(client, ":server 464 :Password incorrect");
		return true;
	}

	client.setPassAccepted(true);
	return (true);
}

bool	CommandHandler::nick(Server &server, Client &client, const Message &msg)
{
	if (msg.getParams().empty())
	{
		server.sendToClient(client, ":server 461 :Not enough parameters");
		return true;
	}
	
	const std::string &nick = msg.getParams()[0];
	
	if(!server.isNicknameAvailable(nick))
	{
		server.sendToClient(client, ":server 433 :Nickname is already in use");
		return true;
	}

	server.registerNickname(client, nick);
}

