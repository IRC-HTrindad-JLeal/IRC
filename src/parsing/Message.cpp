/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 18:30:35 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/10 00:16:02 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Message.h>

Message::Message() {}
Message::~Message() { if (!params.empty()) params.clear(); }

std::string			Message::getMessage() const				{ return message; }
std::string			Message::getCommand() const				{ return command; }
std::vector<std::string>	Message::getParams() const				{ return params; }
std::string			Message::getMiddle() const				{ return middle; }
std::string			Message::getTrailing() const				{ return trailing; }
void				Message::setMessage(const std::string &msg) 		{ message = msg; }
void				Message::setCommand(const std::string &cmd) 		{ command = cmd; }
void				Message::setParams(const std::vector<std::string> &par) { params = par; }
void				Message::setMiddle(const std::string &mid) 		{ middle = mid; }
void				Message::setTrailing(const std::string &trail)		{ trailing = trail; }


static void		setCommand(const std::string &cmd, Message &message, const std::vector<std::string> &cmdLst)
{
	if (find(cmdLst.begin(), cmdLst.end(), cmd) == cmdLst.end())
		throw std::runtime_error("command not found");
	message.setCommand(cmd);
}

static void			thrower() { throw std::runtime_error("parse error"); }

static inline std::size_t	realSize(std::vector<std::string>::const_iterator it, const std::vector<std::string>::const_iterator &end)
{
	std::size_t	size = 1;

	it++;
	for (; it != end; it++)
	{
		size++;
		if ((*it)[0] == ':')
			break ;
	}
	return size;
}

static void			commandParsing(const std::string &cmd, const std::vector<std::string> &p, Message &message)
{
	std::vector<std::string>			param;
	std::vector<std::string>::const_iterator	it = p.begin();
	conditions					cond = NONE;
	bool						channel = false;

	if (p.empty())
		thrower();
	if (cmd == "JOIN" || cmd == "NICK"
		|| cmd == "PASS" || cmd == "PING")
		cond = SINGLE;
	if (cmd == "USER")
		cond = QUAD;
	if (cmd == "CAP")
		cond = STDBLE;
	if (cmd == "MODE")
		cond = DTQUAD;
	if (cmd == "PRIVMSG" || cmd == "INVITE")
		cond = DOUBLE;
	if (cmd == "TOPIC" || cmd == "KICK")
		cond = DBLTRI;
	if (cmd == "JOIN" || cmd == "MODE"
		|| cmd == "KICK" || cmd == "TOPIC")
		channel = true;
	param.push_back(*it);
	if ((cond == SINGLE && p.size() != 1)
		|| (cond == DOUBLE && realSize(it, p.end()) != 2)
		|| (cond == DBLTRI && !(p.size() > 1 && p.size() < 4))
		|| (cond == QUAD && p.size() != 4)
		|| (cond == DTQUAD && !(p.size() > 1 && p.size() < 5))
		|| (channel && (*it)[0] != '#'))
		thrower();
	it++;
	while (it != p.end())
	{
		if ((*it)[0] == ':')
		{
			std::string	colon;
			colon = *it;
			it++;
			for (; it != p.end(); it++)
				colon += ' ' + *it;
			param.push_back(colon);
		}
		else
		{
			param.push_back(*it);
			it++;
		}
	}
	it--;
	if (cmd == "USER" && (*it)[0] != ':')
		thrower();
	if (cond)
		message.setParams(param);
	else
		thrower();
}

static inline void			setParam(std::vector<std::string> &param, Message &message, const std::vector<std::string> &cmdLst)
{
	if (std::isupper(static_cast<unsigned char>(param.front()[0])))
	{
		setCommand(param.front(), message, cmdLst);
		param.erase(param.begin());
		commandParsing(message.getCommand(), param, message);
	}
}

Message					Message::parse(const std::string &raw, const std::vector<std::string> &cmdLst)
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
