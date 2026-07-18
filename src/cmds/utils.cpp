/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 15:28:47 by mely-pan          #+#    #+#             */
/*   Updated: 2026/07/18 19:58:04 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <CommandHandler.h>
#include <Reply.h>

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
	if (!client.tryMarkRegistered())
		return;
	
	server.sendToClient(client, RPL_WELCOME(client.getNickname(), client.getUsername(), client.getIp()));
	server.sendToClient(client, RPL_YOURHOST(client.getNickname()));
	server.sendToClient(client, RPL_CREATED(client.getNickname(), server.getCreationDate())); 
	server.sendToClient(client, RPL_MYINFO(client.getNickname()));
}

const std::string nickOrStar(const Client &client)
{
    const std::string &nick = client.getNickname();
	
    if (nick.empty())
        return "*";
    return nick;
}

std::vector<std::string> CommandHandler::split(const std::string &str, char delimiter)
{
	std::vector<std::string> result;
	std::string item;
	std::stringstream ss(str);

	while (std::getline(ss, item, delimiter))
		result.push_back(item);

	if (!str.empty() && str[str.length() - 1] == delimiter)
		result.push_back("");

	return result;
}
