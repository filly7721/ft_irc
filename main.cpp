#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "ft_irc.h"

Server *g_server = NULL;

void handleSignal(int signal)
{
	if (signal == SIGINT || signal == SIGTERM || signal == SIGQUIT)
		g_server->setStopRunning(true);
}

#include <istream>
int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "Usage: ./ft_irc <port> <password>" << std::endl;
		return 1;
	}

	(void)av;
	while (1)
	{
		std::string str;
		std::getline(std::cin, str);
		printCommand(parseMessage(str));
		// // copyTillSpace test
		// std::cout << "#" << copyTillSpace(str, 0) << "#" << std::endl;
	}


	// signal(SIGINT, handleSignal);
	// signal(SIGTERM, handleSignal);
	// signal(SIGQUIT, handleSignal);

	// uint16_t port = std::atoi(av[1]);
	// std::string password = av[2];
	// (void)password;

	// g_server = new Server(port, password);
	// try
	// {
	// 	g_server->Start();
	// }
	// catch (const std::exception &e)
	// {
	// 	std::cerr << e.what() << '\n';
	// }

	return 0;
}
