/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 22:42:28 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 13:39:55 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <master.h>
#include <Server.h>

int	main(int ac, char **av)
{
	Server	ser;

	if (ac != 3)
	{
		std::cerr << "Insufficient args\n";
		return -1;
	}
	std::cout << "FT_IRC\n";
	try
	{
		signal(SIGINT, Server::handleSig);
		signal(SIGQUIT, Server::handleSig); // Handle the 2 signals
		ser.serverInit(std::atoi(av[1]), av[2]);
	}
	catch (std::exception &e)
	{
		ser.closeFds();
		std::cerr << RED << e.what() << WHI << '\n';
	}
	return 0;
}
