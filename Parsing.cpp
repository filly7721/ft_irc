#include "ft_irc.h"

// copies a word until it reaches a space
std::string	copyTillSpace(const std::string string, size_t start)
{
	size_t len = start;
	while ((len < string.size()) && (isspace(string[len]) == 0))
		len++;
	return (string.substr(start, len - start));
}

// skips a word until it reaches a space
size_t skipWord(const std::string string, size_t start)
{
	size_t len = start;
	while ((len < string.size()) && (isspace(string[len]) == 0))
		len++;
	return (len);
}

Command parseMessage(const std::string &message)
{
	std::vector<std::string>	tokens;
	bool cmd_bool = false;
	Command command;
	size_t i = 0;

    while ((i < message.size()) && (isspace(message[i]) != 0))
		i++;
	
	if (message[i] == ':')
	{
		command.prefix = copyTillSpace(message, i + 1);
		i = skipWord(message, i);
	}

	while (i < message.size())
	{
		while (i < message.size() && isspace(message[i]) != 0)
			i++;
		if (message[i] == ':')
		{
			std::string trail = message.substr(i + 1, message.length() - (i + 1));
			if (trail.find_first_not_of(" \t\n\r") != std::string::npos)
				command.params.push_back(trail);
			return (command);
		}
		if (cmd_bool == false)
		{
			command.cmd = copyTillSpace(message, i);
			i = skipWord(message, i);
			cmd_bool = true;
			continue ;
		}
		else
		{
			command.params.push_back(copyTillSpace(message, i));
			i = skipWord(message, i);
		}
	}
	// check if theres a command, otherwise invalid??????
	return (command);
}

// Command parseMessage(const std::string &message)
// {
// 	std::vector<std::string> tokens;

//     // this method separates trailing ':' words, change it
//     // 
// 	std::stringstream ss(message);
// 	std::string token;

// 	while (ss >> token)
// 	{
// 		tokens.push_back(token);
// 	}
//     //

// 	Command command;
// 	bool	preflag = false;
// 	for (size_t i = 0; i < tokens.size(); i++)
// 	{
// 		if (i == 0)
// 		{
// 			if (tokens[i][0] == ':')
// 			{
// 				command.prefix = tokens[i];
// 				preflag = true;
// 			}
// 			else
// 				command.cmd = tokens[i];
// 		}
// 		else if (i == 1 && preflag == true)
// 			command.cmd = tokens[i];
// 		else
// 			command.params.push_back(tokens[i]);
// 	}
// 	return command;
// }

void	printCommand(Command com)
{
	std::cout << "-----------------------------------" << std::endl;
	std::cout << "prefix: \"" << com.prefix << "\"" << std::endl;
	std::cout << "cmd  : \"" << com.cmd << "\"" << std::endl;
	std::cout << "params :";
	for (std::vector<std::string>::iterator it = com.params.begin(); it < com.params.end() ; it++)
		std::cout << " \"" << *it << "\"";
	std::cout << "\n-----------------------------------" << std::endl;
}