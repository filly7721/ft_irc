#include <unistd.h>

#include <cstring>
#include <iostream>
#include <cstdlib>
#include "Server.hpp"

#define PORT 8080

int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cout << "Usage: ./ft_irc <port> <password>" << std::endl;
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
