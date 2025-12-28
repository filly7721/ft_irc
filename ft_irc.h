#pragma once
#include <csignal>
#include "Server.hpp"

extern Server *g_server;

void handleSignal(int signal)
{
	if (signal == SIGINT || signal == SIGTERM || signal == SIGQUIT)
		g_server->setStopRunning(true);
}
