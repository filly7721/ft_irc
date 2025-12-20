#include <unistd.h>

#include <cstring>
#include <iostream>
#include <cstdlib>
#include "Server.hpp"

int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "Usage: ./ft_irc <port> <password>" << std::endl;
		return 1;
	}
	uint16_t port = std::atoi(av[1]);
	std::string password = av[2];
	(void)password;

	Server server(port);
	try
	{
		server.Start();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

	return 0;
}
