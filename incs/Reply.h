/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Reply.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 21:18:58 by mely-pan          #+#    #+#             */
/*   Updated: 2026/07/15 19:33:22 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Server name (for now)
#define SRV "ft_irc.42"

// Client prefix builder — use when broadcasting to channels
// std::string prefix = USRPREFIX(client);
// broadcastToChannel(ch, RPL_JOIN(prefix, "#canal"), -1);
#define USRPREFIX(c) \
	(":" + (c).getNickname() + "!" + (c).getUsername() + "@" + (c).getIp())

// RPL — Replies

// USER / Registration welcome (send all 4 after PASS+NICK+USER)
#define RPL_WELCOME(nick, user, host) \
	(":" SRV " 001 " + (nick) + " :Welcome to the ft_irc network " + (nick) + "!" + (user) + "@" + (host))

#define RPL_YOURHOST(nick) \
	(":" SRV " 002 " + (nick) + " :Your host is " SRV ", running ft_irc")

#define RPL_CREATED(nick, date) \
	(":" SRV " 003 " + (nick) + " :This server was created " + (date))

#define RPL_MYINFO(nick) \
	(":" SRV " 004 " + (nick) + " " SRV " ft_irc o itkol")

// CAP
#define RPL_CAP_LS(nick) \
	(":" SRV " CAP " + (nick) + " LS :")

#define RPL_CAP_NAK(nick, caps) \
	(":" SRV " CAP " + (nick) + " NAK :" + (caps))

// PING
#define RPL_PONG(token) \
	(":" SRV " PONG " SRV " :" + (token))

// NICK (broadcast)
#define RPL_NICK(prefix, newnick) \
	((prefix) + " NICK " + (newnick))

// QUIT
#define RPL_QUIT(prefix, reason) \
	((prefix) + " QUIT :Quit: " + (reason))

#define RPL_ERROR_QUIT(nick, user, host, reason) \
	("ERROR :Closing link: " + (nick) + "!" + (user) + "@" + (host) + " (Quit: " + (reason) + ")")

// JOIN
#define RPL_JOIN(prefix, channel) \
	((prefix) + " JOIN " + (channel))

#define RPL_NOTOPIC(nick, channel) \
	(":" SRV " 331 " + (nick) + " " + (channel) + " :No topic is set")

#define RPL_TOPIC(nick, channel, topic) \
	(":" SRV " 332 " + (nick) + " " + (channel) + " :" + (topic))

#define RPL_NAMREPLY(nick, channel, names) \
	(":" SRV " 353 " + (nick) + " = " + (channel) + " :" + (names))

#define RPL_ENDOFNAMES(nick, channel) \
	(":" SRV " 366 " + (nick) + " " + (channel) + " :End of /NAMES list")

// PRIVMSG
#define RPL_PRIVMSG(prefix, target, msg) \
	((prefix) + " PRIVMSG " + (target) + " :" + (msg))

// TOPIC change (broadcast)
#define RPL_TOPIC_CHANGE(prefix, channel, topic) \
	((prefix) + " TOPIC " + (channel) + " :" + (topic))

// INVITE
#define RPL_INVITING(nick, target, channel) \
	(":" SRV " 341 " + (nick) + " " + (target) + " " + (channel))

#define RPL_INVITE(prefix, target, channel) \
	((prefix) + " INVITE " + (target) + " :" + (channel))

// KICK
#define RPL_KICK(prefix, channel, target, reason) \
	((prefix) + " KICK " + (channel) + " " + (target) + " :" + (reason))

// MODE
#define RPL_UMODEIS(nick, modestr) \
	(":" SRV " 221 " + (nick) + " " + (modestr))

#define RPL_CHANNELMODEIS(nick, channel, modestr) \
	(":" SRV " 324 " + (nick) + " " + (channel) + " " + (modestr))

#define RPL_MODE(prefix, target, modestr) \
	((prefix) + " MODE " + (target) + " " + (modestr))

// ERR — Errors

// Generic
#define ERR_UNKNOWNCOMMAND(nick, cmd) \
	(":" SRV " 421 " + (nick) + " " + (cmd) + " :Unknown command")

#define ERR_NEEDMOREPARAMS(nick, cmd) \
	(":" SRV " 461 " + (nick) + " " + (cmd) + " :Not enough parameters")

#define ERR_NOTREGISTERED(nick) \
	(":" SRV " 451 " + (nick) + " :You have not registered")

#define ERR_ALREADYREGISTERED(nick) \
	(":" SRV " 462 " + (nick) + " :You may not reregister")

// PASS
#define ERR_PASSWDMISMATCH(nick) \
	(":" SRV " 464 " + (nick) + " :Password incorrect")

// NICK
#define ERR_NONICKNAMEGIVEN(nick) \
	(":" SRV " 431 " + (nick) + " :No nickname given")

#define ERR_ERRONEUSNICKNAME(nick, badnick) \
	(":" SRV " 432 " + (nick) + " " + (badnick) + " :Erroneous nickname")

#define ERR_NICKTOOLONG(client, nick) \
	(":" SRV " 432 " + (client) + " " + (nick) + " :Nickname too long, max. 9 characters")

#define ERR_NICKNAMEINUSE(nick, badnick) \
	(":" SRV " 433 " + (nick) + " " + (badnick) + " :Nickname is already in use")

// Channel existence / membership
#define ERR_NOSUCHCHANNEL(nick, channel) \
	(":" SRV " 403 " + (nick) + " " + (channel) + " :No such channel")

#define ERR_NOTONCHANNEL(nick, channel) \
	(":" SRV " 442 " + (nick) + " " + (channel) + " :You're not on that channel")

#define ERR_USERNOTINCHANNEL(nick, target, channel) \
	(":" SRV " 441 " + (nick) + " " + (target) + " " + (channel) + " :They aren't on that channel")

#define ERR_USERONCHANNEL(nick, target, channel) \
	(":" SRV " 443 " + (nick) + " " + (target) + " " + (channel) + " :is already on channel")

// Channel access
#define ERR_CHANNELISFULL(nick, channel) \
	(":" SRV " 471 " + (nick) + " " + (channel) + " :Cannot join channel (+l)")

#define ERR_INVITEONLYCHAN(nick, channel) \
	(":" SRV " 473 " + (nick) + " " + (channel) + " :Cannot join channel (+i)")

#define ERR_BADCHANNELKEY(nick, channel) \
	(":" SRV " 475 " + (nick) + " " + (channel) + " :Cannot join channel (+k)")

// Privileges
#define ERR_CHANOPRIVSNEEDED(nick, channel) \
	(":" SRV " 482 " + (nick) + " " + (channel) + " :You're not channel operator")

// PRIVMSG
#define ERR_NOSUCHNICK(nick, target) \
	(":" SRV " 401 " + (nick) + " " + (target) + " :No such nick/channel")

#define ERR_NORECIPIENT(nick, cmd) \
	(":" SRV " 411 " + (nick) + " :No recipient given (" + (cmd) + ")")

#define ERR_NOTEXTTOSEND(nick) \
	(":" SRV " 412 " + (nick) + " :No text to send")

#define ERR_CANNOTSENDTOCHAN(nick, channel) \
	(":" SRV " 404 " + (nick) + " " + (channel) + " :Cannot send to channel")

// MODE
#define ERR_UNKNOWNMODE(nick, modechar) \
	(":" SRV " 472 " + (nick) + " " + (modechar) + " :is unknown mode char")

#define ERR_USERSDONTMATCH(nick) \
	(":" SRV " 502 " + (nick) + " :Cant change mode for other users")

