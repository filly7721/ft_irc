#pragma once
#include <csignal>
#include <sstream>
#include <vector>
#include "Server.hpp"

class Server;

extern Server *g_server;

struct Command
{
	std::string prefix;
	std::string cmd;
	std::vector<std::string> params;
};

// parsing
Command		parseMessage(const std::string &message);
void		printCommand(Command com);
std::string	copyTillSpace(const std::string string, size_t start);
size_t		skipWord(const std::string string, size_t start);
