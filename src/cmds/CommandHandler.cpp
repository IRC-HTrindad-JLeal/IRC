/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:55:39 by mely-pan          #+#    #+#             */
/*   Updated: 2026/07/18 20:05:42 by mely-pan         ###   ########.fr       */
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
		server.sendToClient(client, ERR_NOTREGISTERED(nickOrStar(client)));
		return true;
	}
	std::map<std::string, CommandFt>::iterator it = _handlers.find(cmd);

	if (it == _handlers.end())
	{
		server.sendToClient(client, ERR_UNKNOWNCOMMAND(replyNick(client), msg.getCommand()));
		return true;
	}
	if (requiresAuth(cmd) && !client.isRegistered())
	{
		server.sendToClient(client, ERR_NOTREGISTERED(replyNick(client)));
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
	if (msg.paramCount() == 0)
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(nickOrStar(client), msg.getCommand()));
		return true;
	}
	if (client.isPassAccepted())
		return true;
	if (msg.getMiddle() != server.getPassword())
	{
		server.sendToClient(client, ERR_PASSWDMISMATCH(nickOrStar(client)));
		return true;
	}
	client.setPassAccepted(true);
	return (true);
}

bool	CommandHandler::nick(Server &server, Client &client, const Message &msg)
{
	if (!client.isPassAccepted())
    {
        server.sendToClient(client, ERR_NOTREGISTERED(nickOrStar(client)));
        return true;
    }
	if (msg.getParams().empty())
	{
		server.sendToClient(client, ERR_NONICKNAMEGIVEN(nickOrStar(client)));
		return true;
	}
	
	const std::string &nick = msg.getMiddle();
	
	if (nick == client.getNickname())
		return true;
	if(!server.isNicknameAvailable(nick))
	{
		server.sendToClient(client, ERR_NICKNAMEINUSE(nickOrStar(client), nick));
		return true;
	}
	if(!server.isNicknameAvailable(nick))
	{
		server.sendToClient(client, ERR_ERRONEUSNICKNAME(nickOrStar(client), nick));
		return true;
	}
	if (nick.size() > 9)
	{
		server.sendToClient(client, ERR_NICKTOOLONG(nickOrStar(client), nick));
		return true;
	}
	if (client.isRegistered())
	{
		std::string	prefix = USRPREFIX(client);
		server.registerNickname(client, nick);
		server.sendToClient(client, RPL_NICK(prefix, nick));
		server.broadcastToClientChannels(client, RPL_NICK(prefix, nick), client.getFd());
	}
	else
	{
		server.registerNickname(client, nick);
		tryRegistration(server, client);
	}
	return true;
}

bool	CommandHandler::user(Server &server, Client &client, const Message &msg)
{
	if (!client.isPassAccepted())
	{
		server.sendToClient(client, ERR_NOTREGISTERED(nickOrStar(client)));
		return true;
	}
	if (client.isRegistered())
	{
		server.sendToClient(client, ERR_ALREADYREGISTERED(client.getNickname()));
		return true;
	}

	const std::vector<std::string> &p = msg.getParams();

	if(msg.paramCount() < 4)
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(nickOrStar(client), msg.getCommand()));
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
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(nickOrStar(client), "PING"));
		return true;
	}
	server.sendToClient(client, RPL_PONG(msg.getMiddle()));
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
	const std::vector<std::string> &params = msg.getParams();
	const std::string &nick = client.isRegistered() ? client.getNickname() : "*";

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

static bool   isJoinChannelName(const std::string &name)
{
	if (name.empty())
		return false;
	if (name[0] != '#')
		return false;
	if (name.length() == 1)
		return false;
	if (name.length() > 50)
		return false;
	if (name.find(' ') != std::string::npos)
		return false;
	if (name.find('\a') != std::string::npos)
		return false;
	return true;
}

static void welcomeMsg(Server &server, Client &client, Channel &channel)
{
	std::string			prefix = RPL_NAMREPLY(client.getNickname(), channel.getName(), "");

	if (prefix.size() + 2 > LINE_LEN_BUF_MAX)
	{
		server.sendToClient(client, RPL_ENDOFNAMES(client.getNickname(), channel.getName()));
		return;
	}

	const std::map<int, bool>	&membMap = channel.getMembers();
	size_t						maxPayload = LINE_LEN_BUF_MAX - prefix.size() - 2;
	std::string 				line = "";

	for (std::map<int, bool>::const_iterator it = membMap.begin(); it != membMap.end(); ++it)
	{
		Client * member = server.findClientByFd(it->first);
		std::string name = "";
		if (member)
		{
			if (it->second)
				name += "@";
			name += member->getNickname();
		}
		else
		{
			// TODO
			// handle very unlikely edge case where a client exists in a channel but is not registered on the server.
			continue;
		}

		std::string next = line.empty() ? name : line + " " + name;
		if (next.size() > maxPayload && !line.empty())
		{
			server.sendToClient(client, prefix + line);
			line = name;
		}
		else 
			line = next;
	}
	server.sendToClient(client, prefix + line);
	server.sendToClient(client, RPL_ENDOFNAMES(client.getNickname(), channel.getName()));
}

bool CommandHandler::join(Server &server, Client &client, const Message &msg)
{
	const std::vector<std::string> &params = msg.getParams();

	if (params.empty())
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), "JOIN"));
		return true;
	}

	std::vector<std::string> channels = split(params[0], ',');
	std::vector<std::string> keys;

	if (params.size() == 2)
		keys = split(params[1], ',');

	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::string target = channels[i];
		if (!isJoinChannelName(target))
		{
			server.sendToClient(client, ERR_NOSUCHCHANNEL(client.getNickname(), target));
			continue;
		}
		Channel &channel = server.getOrCreateChannel(target);
		bool firstUser = channel.empty();

		if (channel.hasMember(client.getFd()))
			continue;

		if (channel.isFull())
		{
			server.sendToClient(client, ERR_CHANNELISFULL(client.getNickname(), target));
			continue;
		}
		if (channel.isInviteOnly())
		{
			if (!channel.isInvited(client.getFd()))
			{
				server.sendToClient(client, ERR_INVITEONLYCHAN(client.getNickname(), target));
				continue;
			}
		}
		if (channel.hasKey())
		{
			if (i >= keys.size() || keys[i] != channel.getKey())
			{
				server.sendToClient(client, ERR_BADCHANNELKEY(client.getNickname(), target));
				continue;
			}
		}

		if (channel.addMember(client.getFd(), firstUser))
			server.broadcastToChannel(channel, RPL_JOIN(USRPREFIX(client), target), -1);
		else
		{
			// TODO
			// handle case where addMember fails
			return (true);
		}

		if (channel.isInvited(client.getFd()))
			channel.uninvite(client.getFd());

		if (channel.getTopic().empty())
			server.sendToClient(client, RPL_NOTOPIC(client.getNickname(), target));
		else
			server.sendToClient(client, RPL_TOPIC(client.getNickname(), target, channel.getTopic()));
		welcomeMsg(server, client, channel);
	}
	return (true);
}

bool CommandHandler::privmsg(Server &server, Client &client, const Message &msg)
{
	if (msg.paramCount() < 1)
	{
		server.sendToClient(client, ERR_NORECIPIENT(client.getNickname(), "PRIVMSG"));
		return true;
	}
	if (msg.paramCount() < 2)
	{
		server.sendToClient(client, ERR_NOTEXTTOSEND(client.getNickname()));
		return true;
	}

	const std::string &target = msg.getParam(0);
	const std::string &text = msg.getParam(1);

	if (text.empty())
	{
		server.sendToClient(client, ERR_NOTEXTTOSEND(client.getNickname()));
		return true;
	}

	std::string reply = RPL_PRIVMSG(USRPREFIX(client), target, text);

	if (target[0] == '#')
	{
		Channel *targetChannel = server.findChannel(target);
		if (!targetChannel)
		{
			server.sendToClient(client, ERR_NOSUCHCHANNEL(client.getNickname(), target));
			return true;
		}
		if (!targetChannel->hasMember(client.getFd()))
		{
			server.sendToClient(client, ERR_CANNOTSENDTOCHAN(client.getNickname(), target));
			return true;
		}
		server.broadcastToChannel(*targetChannel, reply, client.getFd());
	}
	else
	{
		Client *targetNick = server.findClientByNickname(target);

		if (!targetNick) {
			server.sendToClient(client, ERR_NOSUCHNICK(client.getNickname(), target));
			return true;
		}

		server.sendToClient(*targetNick, reply);
	}

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


