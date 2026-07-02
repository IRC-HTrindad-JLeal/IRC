/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:55:39 by mely-pan          #+#    #+#             */
/*   Updated: 2026/07/01 17:40:27 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <CommandHandler.h>
#include <Reply.h>

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
		server.sendToClient(client, ERR_NOTREGISTERED(client.getNickname()));
		return true;
	}
	std::map<std::string, CommandFt>::iterator it = _handlers.find(cmd);

	if (it == _handlers.end())
	{
		server.sendToClient(client, ERR_UNKNOWNCOMMAND(client.getNickname(), msg.getCommand()));
		return true;
	}
	return (this->*(it->second))(server, client, msg);
}

bool	CommandHandler::pass(Server &server, Client &client, const Message &msg)
{
	if (client.isRegistered())
	{
		server.sendToClient(client, ERR_ALREADYREGISTERED(client.getNickname()));
		return true;
	}
	if (msg.getParams().empty())
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), msg.getCommand()));
		return true;
	}
	if (client.isPassAccepted())
		return true;
	if (msg.getParams()[0] != server.getPassword())
	{
		server.sendToClient(client, ERR_PASSWDMISMATCH(client.getNickname()));
		return true;
	}
	client.setPassAccepted(true);
	tryRegistration(server, client);
	return (true);
}

bool	CommandHandler::nick(Server &server, Client &client, const Message &msg)
{
	if (msg.getParams().empty())
	{
		server.sendToClient(client, ERR_NONICKNAMEGIVEN(client.getNickname()));
		return true;
	}
	
	// TODO: change this variable back into a reference once the Message getters are updated.
	//const std::string &nick = msg.getParams()[0];
	const std::string nick = msg.getParams()[0];
	
	if(!server.isNicknameAvailable(nick))
	{
		server.sendToClient(client, ERR_NICKNAMEINUSE(client.getNickname(), nick));
		return true;
	}
	if (!nickValid(nick))
	{
		server.sendToClient(client, ERR_ERRONEUSNICKNAME(client.getNickname(), nick));
		return true;
	}
	// broadcast("<oldnick>!user@host NICK <nick>"); 
	server.registerNickname(client, nick);
	tryRegistration(server, client);
	return true;
}

bool	CommandHandler::user(Server &server, Client &client, const Message &msg)
{
	(void)server;

	if (client.isRegistered())
	{
		server.sendToClient(client, ERR_ALREADYREGISTERED(client.getNickname()));
		return true;
	}

	std::vector<std::string> p = msg.getParams();

	if(p.size() < 4)
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), msg.getCommand()));
		return true;
	}
	client.setUsername(p[0]);
	std::string realname = p[3];

	if (!realname.empty() && realname[0] == ':')
		realname.erase(0, 1);
	client.setRealname(realname);
	tryRegistration(server, client);
	return true;
}

bool	CommandHandler::ping(Server &server, Client &client, const Message &msg)
{
	if (msg.getParams().empty())
		return true;
	server.sendToClient(client, RPL_PONG(msg.getParams()[0]));
	return true;
}

bool	CommandHandler::quit(Server &server, Client &client, const Message &msg)
{
	(void)server;
	(void)client;
	(void)msg;
	return false;
}

// TODO
// Implemented minial cap for testing, needs review
bool CommandHandler::cap(Server &server, Client &client, const Message &msg)
{
	std::vector<std::string> params = msg.getParams();
	std::string nick = client.getNickname();

	if (nick.empty())
		nick = "*";
	if (params.empty())
		return (true);
	if (params[0] == "LS")
	{
		server.sendToClient(client, RPL_CAP_LS(nick));
		return (true);
	}
	if (params[0] == "REQ")
	{
		if (params.size() > 1)
			server.sendToClient(client, RPL_CAP_NAK(nick, params[1]));
		else
			server.sendToClient(client, RPL_CAP_NAK(nick, ""));
		return (true);
	}
	if (params[0] == "END")
		return (true);
    return (true);
}

bool CommandHandler::pong(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}

bool CommandHandler::join(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}

bool CommandHandler::privmsg(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}

bool CommandHandler::mode(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}

bool CommandHandler::topic(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}

bool CommandHandler::invite(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}

bool CommandHandler::kick(Server &server, Client &client, const Message &msg)
{
    (void)server; (void)client; (void)msg;
    return (true); // TODO
}


