/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 15:28:47 by mely-pan          #+#    #+#             */
/*   Updated: 2026/07/01 17:54:40 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <CommandHandler.h>
#include <Reply.h>
#include <ctime>

bool	CommandHandler::nickValid(const std::string &nick)
{
	if (nick[0] == '$' || nick[0] == ':' || nick[0] == '#' || nick[0] == '&')
		return false;
	for (size_t i = 0; i < nick.length(); i++)
	{
		if (nick[i] == ' ' ||
			nick[i] == ',' ||
			nick[i] == '*' ||
			nick[i] == '@' ||
			nick[i] == '!' ||
			nick[i] == '?')
		{
			return false;
		}
	}
	return true;
}

void	CommandHandler::tryRegistration(Server &server, Client &client)
{
	if (!client.isRegistered())
		return ;
	
	server.sendToClient(client, RPL_WELCOME(client.getNickname(), client.getUsername(), client.getIp()));
	server.sendToClient(client, RPL_YOURHOST(client.getNickname()));
	server.sendToClient(client, RPL_CREATED(client.getNickname(), "123")); // ! add get_date function instead of "123" 
	server.sendToClient(client, RPL_MYINFO(client.getNickname()));
}
