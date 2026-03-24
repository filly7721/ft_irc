#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "ft_irc.h"
#include "Server.hpp"

Server *g_server = NULL;

Command parseMessage(const std::string &message)
{
	Command command;
	std::string line = message;
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);
	if (line.empty())
		return command;

	std::stringstream ss(line);
	std::string token;
	if (!(ss >> command.name))
		return command;

	while (ss >> token)
	{
		if (!token.empty() && token[0] == ':')
		{
			token.erase(0, 1);
			std::string trailing;
			std::getline(ss, trailing);
			if (!trailing.empty() && trailing[0] == ' ')
				trailing.erase(0, 1);
			if (!trailing.empty())
				token += " " + trailing;
			command.params.push_back(token);
			break;
		}
		else
			command.params.push_back(token);
	}
	return command;
}

void handleSignal(int signal)
{
	if (signal == SIGINT || signal == SIGTERM || signal == SIGQUIT)
	{
		if (g_server)
			g_server->setStopRunning(true);
	}
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

	int port = std::atoi(av[1]);
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
	delete g_server;
	return 0;
}
