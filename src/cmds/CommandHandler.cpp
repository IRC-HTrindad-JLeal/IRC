/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:55:39 by mely-pan          #+#    #+#             */
/*   Updated: 2026/06/24 18:17:50 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <CommandHandler.h>

CommandHandler::CommandHandler()
{
	inItHandlers();
}

CommandHandler::~CommandHandler() {}

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
		server.sendToClient(client, ":server 451 :You have not registered");
		return true;
	}
	std::map<std::string, CommandFt>::iterator it = _handlers.find(cmd);

	if (it == _handlers.end())
	{
		server.sendToClient(client, ":server 421 :Uknown command");
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
	return true;
}

bool	CommandHandler::user(Server &server, Client &client, const Message &msg)
{
	(void)server;

	std::vector<std::string> p = msg.getParams();

	if(p.size() < 4)
	{
		server.sendToClient(client, ":server 461 USER :Not enough params");
		return true;
	}
	client.setUsername(p[0]);
	std::string realname = p[3];

	if (!realname.empty() && realname[0] == ':')
		realname.erase(0, 1);
	client.setRealname(realname);
	return true;
}

bool	CommandHandler::ping(Server &server, Client &client, const Message &msg)
{
	if (msg.getParams().empty())
		return true;
	server.sendToClient(client, "PONG :" + msg.getParams()[0]);
	return true;
}

bool	CommandHandler::quit(Server &server, Client &client, const Message &msg)
{
	(void)server;
	(void)client;
	(void)msg;
	return false;
}

bool CommandHandler::cap(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::pong(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::join(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::privmsg(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::mode(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::topic(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::invite(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}

bool CommandHandler::kick(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (false); // TODO
}


