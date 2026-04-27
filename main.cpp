#include <unistd.h>
#include <cctype>
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

	size_t pos = 0;

	// Optional prefix: ":prefix "
	if (line[0] == ':')
	{
		size_t end = line.find(' ');
		if (end == std::string::npos)
			return command;
		command.prefix = line.substr(1, end - 1);
		pos = line.find_first_not_of(' ', end);
		if (pos == std::string::npos)
			return command;
	}

	// Command name — folded to uppercase (IRC is case-insensitive for commands)
	size_t end = line.find(' ', pos);
	std::string name = (end == std::string::npos) ? line.substr(pos) : line.substr(pos, end - pos);
	for (size_t i = 0; i < name.size(); ++i)
		name[i] = std::toupper(static_cast<unsigned char>(name[i]));
	command.name = name;
	if (end == std::string::npos)
		return command;
	pos = line.find_first_not_of(' ', end);

	// Parameters: middle params, then optional ":trailing" (may contain spaces)
	while (pos != std::string::npos && pos < line.size())
	{
		if (line[pos] == ':')
		{
			command.params.push_back(line.substr(pos + 1));
			break;
		}
		end = line.find(' ', pos);
		if (end == std::string::npos)
		{
			command.params.push_back(line.substr(pos));
			break;
		}
		command.params.push_back(line.substr(pos, end - pos));
		pos = line.find_first_not_of(' ', end);
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
