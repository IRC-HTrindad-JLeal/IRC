/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:55:39 by mely-pan          #+#    #+#             */
/*   Updated: 2026/07/31 00:09:11 by mely-pan         ###   ########.fr       */
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
	if (msg.getMiddle() != server.getPassword())
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
	const std::string nick = msg.getMiddle();
	
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

static std::string	buildModeStr(const Channel &channel)
{
	std::string	modes = "+";
	std::string	params = "";

	if (channel.isInviteOnly())
		modes += "i";
	if (channel.isTopicOnly())
		modes += "t";
	if (!channel.getKey().empty())
	{
		modes += "k";
		params += " " + channel.getKey();
	}
	if (channel.getUserLimit() > 0)
	{
		modes += "l";
		std::ostringstream	oss;
		oss << channel.getUserLimit();
		params += " " + oss.str();
	}
	if (modes == "+")
		return "";
	return modes + params;
}

bool CommandHandler::mode(Server &server, Client &client, const Message &msg)
{
	const std::vector<std::string> params = msg.getParams();
	
    if (params.empty())
	{
		server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), "MODE"));
		return true;
	}
	
	const std::string &target = params[0];

	//user MODE
	if (target[0] != '#')
	{
		if (target != client.getNickname())
		{
			server.sendToClient(client, ERR_USERSDONTMATCH(client.getNickname()));
			return (true);
		}
		if (params.size() < 2)
			server.sendToClient(client, RPL_UMODEIS(client.getNickname(), "+"));
		return true ;
	}
	
	//Channel MODE
	Channel	*chan = server.findChannel(target);

	if (!chan)
	{
		server.sendToClient(client, ERR_NOSUCHCHANNEL(client.getNickname(), target));
		return true;
	}
	if (!chan->hasMember(client.getFd()))
	{
		server.sendToClient(client, ERR_NOTONCHANNEL(client.getNickname(), target));
		return true;
	}
	if (params.size() < 2) //MODE #chan
	{
		server.sendToClient(client, RPL_CHANNELMODEIS(client.getNickname(), target, buildModeStr(*chan)));
		return true;
	}
	if (!chan->isOperator(client.getFd()))
	{
		server.sendToClient(client, ERR_CHANOPRIVSNEEDED(client.getNickname(), target));
		return true;
	}
	
	//parsing
	const std::string	&modestr = params[1];
	size_t				paramIdx = 2;
	char				sign = '+';
	std::string			appliedPlus = "";
	std::string			appliedMinus = "";
	std::string			appliedPlusParams = "";
	std::string			appliedMinusParams = "";
	bool				hasSign = false;

	for (size_t i = 0; i < modestr.size(); ++i)
	{
		char c = modestr[i];

		if (c == '+' || c == '-')
		{
			sign = c;
			hasSign = true;
			continue ;
		}
		if (!hasSign)
			continue;
		if (c == 'i')
		{
    		chan->setInviteOnly(sign == '+');
    		if (sign == '+')
				appliedPlus += c;
    		else
				appliedMinus += c;
		}
		else if (c == 't')
		{
			chan->setTopicOnly(sign == '+');
			if (sign == '+')
				appliedPlus += c;
			else
				appliedMinus += c;
		}
		else if (c == 'k')
		{
			if (sign == '+')
			{
				if (paramIdx >= params.size())
				{
					server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), "MODE"));
					continue;
				}
				chan->setKey(params[paramIdx]);
				appliedPlus += c;
				appliedPlusParams += " " + params[paramIdx];
				paramIdx++;
			}
			else
			{
				chan->clearKey();
				appliedMinus += c;
			}
		}
		else if (c == 'o')
		{
			if (paramIdx >= params.size())
			{
				server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), "MODE"));
				continue;
			}
			
			Client *target_client = server.findClientByNickname(params[paramIdx]);
			
			if (!target_client || !chan->hasMember(target_client->getFd()))
			{
				server.sendToClient(client, ERR_USERNOTINCHANNEL(client.getNickname(), params[paramIdx], target));
				paramIdx++;
				continue;
			}
			if (sign == '+')
			{
				appliedPlus += c;
				appliedPlusParams += " " + params[paramIdx]; 
			}
			else
			{
				appliedMinus += c;
				appliedMinusParams += " " + params[paramIdx];
			}
			chan->setOperator(target_client->getFd(), sign == '+');
			paramIdx++;
		}
		else if (c == 'l')
		{
			if (sign == '+')
			{
				if (paramIdx >= params.size())
				{
					server.sendToClient(client, ERR_NEEDMOREPARAMS(client.getNickname(), "MODE"));
					continue ;
				}
				//validate all the char are digits
				const std::string	&limitStr = params[paramIdx];
				bool				valid = !limitStr.empty();

				for (size_t j = 0; limitStr.size() > j; j++)
				{
					if (!std::isdigit(limitStr[j]))
					{
						valid = false;
						break ;
					}
				}
				if (!valid)
				{
					paramIdx++;
					continue ;
				}
				
				std::istringstream	iss(limitStr);
				size_t				limit = 0;
				
				iss >> limit;
				if (iss.fail() || !iss.eof() || limit == 0)
				{
					paramIdx++;
					continue ;
				}
				chan->setUserLimit(limit);
				appliedPlusParams += " " + limitStr;
				appliedPlus += c;
				paramIdx++;
			}
			else
			{
				chan->clearUserLimit();
				appliedMinus += c;
			}
		}
		else
			server.sendToClient(client, ERR_UNKNOWNMODE(client.getNickname(), std::string(1, c)));
	}

	std::string modeChange = "";
	
	if (!appliedPlus.empty())
	    modeChange += "+" + appliedPlus + appliedPlusParams;
	if (!appliedMinus.empty())
	    modeChange += "-" + appliedMinus + appliedMinusParams;
	if (!modeChange.empty())
	    server.broadcastToChannel(*chan, RPL_MODE(USRPREFIX(client), target, modeChange), -1);
    return (true);
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


