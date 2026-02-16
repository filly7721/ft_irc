#include "ft_irc.h"

// copies a word until it reaches a space
std::string	copyTillSpace(const std::string string, size_t start)
{
	size_t len = start;
	while ((len < string.size()) && (isspace(string[len]) == false))
		len++;
	return (string.substr(start, len - start));
}

// skips a word until it reaches a space
size_t skipWord(const std::string string, size_t start)
{
	size_t len = start;
	while ((len < string.size()) && (isspace(string[len]) == false))
		len++;
	return (len);
}

Command parseMessage(const std::string &message)
{
	std::vector<std::string>	tokens;
	size_t i = 0;
    while ((i < message.size()) && (isspace(message[i]) == true))
		i++;
	
	Command command;
	
	if (message[i] == ':')
		command.prefix.push_back(copyTillSpace(message, i + 1));

	while (i < message.size())
	{
		while (isspace(message[i]) == true)
			i++;
		if (message[i] == ':')
		{
			command.params.push_back(string.substr(i + 1, message.length() - (i + 1)));
			return (command);
		}
		command.prefix.push_back(copyTillSpace(message, i));
		i = skipWord(message, i);
	}
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
// 				command.cmnd = tokens[i];
// 		}
// 		else if (i == 1 && preflag == true)
// 			command.cmnd = tokens[i];
// 		else
// 			command.params.push_back(tokens[i]);
// 	}
// 	return command;
// }

void	printCommand(Command com)
{
	std::cout << "-----------------------------------" << std::endl;
	std::cout << "prefix: \"" << com.prefix << "\"" << std::endl;
	std::cout << "cmnd  : \"" << com.cmnd << "\"" << std::endl;
	std::cout << "params :";
	for (std::vector<std::string>::iterator it = com.params.begin(); it < com.params.end() ; it++)
		std::cout << " \"" << *it << "\"";
	std::cout << "\n-----------------------------------" << std::endl;
}