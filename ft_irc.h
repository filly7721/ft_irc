#pragma once
#include <csignal>
#include <sstream>
#include <string>
#include <vector>

class Server;

extern Server *g_server;

struct Command
{
	std::string prefix;
	std::string name;
	std::vector<std::string> params;
};

Command parseMessage(const std::string &message);