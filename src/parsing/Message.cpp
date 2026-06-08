/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 18:30:35 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/09 00:14:51 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Message.h>

Message::Message() {}
Message::~Message()
{
	if (!params.empty())
		params.clear();
}

std::string			Message::getMessage() const				{ return message; }
std::string			Message::getPrefix() const				{ return prefix; }
std::string			Message::getCommand() const				{ return command; }
std::vector<std::string>	Message::getParams() const				{ return params; }
std::string			Message::getMiddle() const				{ return middle; }
std::string			Message::getTrailing() const				{ return trailing; }
void				Message::setMessage(const std::string &msg) 		{ message = msg;}
void				Message::setPrefix(const std::string &pre) 		{prefix = pre; }
void				Message::setCommand(const std::string &cmd) 		{ command = cmd; }
void				Message::setParams(const std::vector<std::string> &par) { params = par; }
void				Message::setMiddle(const std::string &mid) 		{ middle = mid; }


static void		setCommand(const std::string &cmd, Message &message, bool isnum, const std::vector<std::string> &cmdLst)
{
	if (isnum)
	{
		if (cmd.length() != 3)
			throw std::runtime_error("not a valid command digit");
		for (std::size_t i = 1; i < 3; i++)
			if (!std::isdigit(static_cast<unsigned char>(cmd[i])))
				throw std::runtime_error("not a digit");
		message.setCommand(cmd);
	}
	else 
		if (find(cmdLst.begin(), cmdLst.end(), cmd) == cmdLst.end())
			throw std::runtime_error("command not found");
	message.setCommand(cmd);
}

static void		thrower() { throw std::runtime_error("parse error"); }

static void		commandParsing(const std::string &cmd, const std::vector<std::string> &p, Message &message)
{
	std::vector<std::string>			param;
	std::vector<std::string>::const_iterator	it;
	uint8_t						flags = 0;

	it = p.begin();
	if (p.size() < 1)
		thrower();
	if (cmd == "JOIN" || cmd == "MODE")
	{
		flags |= 1;
		if (p.front()[0] != '#')
			thrower();
	}
	if (cmd == "MODE" || cmd == "PRIVMSG")
		flags |= (1 << 1);
	if (cmd == "USER")
		flags |= (1 << 2);
	if (cmd == "MODE")
		flags |= (1 << 3);
	param.push_back(*it);
	it++;
	if ((flags & 1 && (*it)[0] != '#')
		|| (flags & (1 << 1) && p.size() < 3)
		|| (flags & (1 << 2) && p.size() < 2))
		thrower();
	while (it != p.end())
	{
		param.push_back(*it);
		it++;
	}
	message.setParams(param);
}

static inline void		setParam(std::vector<std::string> &param, Message &message, const std::vector<std::string> &cmdLst)
{
	if (std::isupper(static_cast<unsigned char>(param.front()[0])))
	{
		setCommand(param.front(), message, std::isdigit(static_cast<unsigned char>(param.front()[0])), cmdLst);
		param.erase(param.begin());
	}
}

Message				Message::parse(const std::string &raw, const std::vector<std::string> &cmdLst)
{
	Message				message;
	std::vector<std::string>	params;
	std::stringstream		ss(raw);
	std::string			buffer;

	while (ss >> buffer)
		params.push_back(buffer);
	// step 1
	setParam(params, message, cmdLst);
	return message;
}
