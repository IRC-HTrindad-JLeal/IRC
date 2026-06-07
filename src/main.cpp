/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 22:42:28 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/07 03:38:31 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <master.h>
#include <Server.h>

int	main(int ac, char **av)
{
	Server	ser;

	try
	{
		signal(SIGINT, Server::handleSig);
		signal(SIGQUIT, Server::handleSig); // Handle the 2 signals
		ser.serverInit(); // Initialize
	}
	catch (std::exception &e)
	{
		ser.closeFds();
		std::cerr << e.what() <<'\n';
	}
	return 0;
}
