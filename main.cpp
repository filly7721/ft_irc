#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "ft_irc.h"

Server *g_server = NULL;

Command parseMessage(const std::string &message)
{
	std::vector<std::string> tokens;
	std::stringstream ss(message);
	std::string token;
	while (ss >> token)
	{
		tokens.push_back(token);
	}
	Command command;
	for (size_t i = 0; i < tokens.size(); i++)
	{
		if (i == 0)
			command.name = tokens[i];
		else
			command.params.push_back(tokens[i]);
	}
	return command;
}

void handleSignal(int signal)
{
	if (signal == SIGINT || signal == SIGTERM || signal == SIGQUIT)
		g_server->setStopRunning(true);
}

int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "Usage: ./ft_irc <port> <password>" << std::endl;
		return 1;
	}

	signal(SIGINT, handleSignal);
	signal(SIGTERM, handleSignal);
	signal(SIGQUIT, handleSignal);

	uint16_t port = std::atoi(av[1]);
	std::string password = av[2];
	(void)password;

	g_server = new Server(port, password);
	try
	{
		g_server->Start();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

	return 0;
}
