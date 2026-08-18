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

static void setUpSignals()
{
	struct sigaction sa;
	struct sigaction ignore;

	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = Server::handleSig;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1)
		throw std::runtime_error("sigaction(SIGINT) failed");
	if (sigaction(SIGQUIT, &sa, NULL) == -1)
		throw std::runtime_error("sigaction(SIGQUIT) failed");

	std::memset(&ignore, 0, sizeof(ignore));
	ignore.sa_handler = SIG_IGN;
	sigemptyset(&ignore.sa_mask);
	ignore.sa_flags = 0;

	if (sigaction(SIGPIPE, &ignore, NULL) == -1)
		throw std::runtime_error("sigaction(SIGPIPE) failed");
}

static int parsePort(const char* arg)
{
	char    *end;
	long    port;

	if (!arg || !*arg)
		throw std::runtime_error("Port must be numeric");

	for (int i = 0; arg[i]; ++i)
	{
		if (arg[i] < '0' || arg[i] > '9')
			throw std::runtime_error("Port must be numeric");
	}

	errno = 0;
	port = std::strtol(arg, &end, 10);

	if (errno == ERANGE || *end != '\0')
		throw std::runtime_error("Invalid port");
	if (port <= 0 || port > PORT_MAX)
		throw std::runtime_error("Port number out of bounds");

	return static_cast<int>(port);
}

int	main(int ac, char **av)
{
	Server	ser;

	if (ac != 3)
	{
		std::cerr << "Insufficient args\n";
		return -1;
	}
	try
	{
		setUpSignals();
		ser.serverInit(parsePort(av[1]), av[2]);
	}
	catch (std::exception &e)
	{
		ser.closeFds();
		std::cerr << RED << e.what() << WHI << '\n';
		return 1;
	}
	return 0;
}

