#include "Client.hpp"
#include <algorithm>
#include <Server.hpp>
#include "Channel.hpp"

std::vector<std::string> getArgs(std::string commandLine) {
	std::vector<std::string> args;
	while (commandLine.length()) {
		std::string nextArg;
		// if ':' is found, take the remaining chars of the string as one argument
		if (commandLine[0] == ':') {
			commandLine.erase(0, 1);
			nextArg = commandLine;
		}
		// else take until whitespace
		else
			nextArg = commandLine.substr(0, commandLine.find(" "));
		// remove argument taken from the command line
		commandLine.erase(0, nextArg.length());
		// skip all spaces
		while (commandLine[0] == ' ')
			commandLine.erase(0, 1);
		// skip arg if empty
		if (nextArg.length())
			args.push_back(nextArg);
	}
	return args;
}

void Client::pass(std::string &commandLine) { // 100% finished
	std::vector<std::string> args = getArgs(commandLine);
	// no params
	if (!args.size())
		logger.warn("461 " + _nickName + " :Not enough parameters");
	_password = args[0];
	logger.info("password set to : [" + _password + "]");
	_authorized = true;
	// comparing outputted pw with server pw
	if (_password != PASSWORD) {
		// nn hh
		_authorized = false;
		std::string e = "464 " + _nickName + " :Password incorrect";
		logger.warn(e);
	}
	/* we dont knoe which auth command will be executed the last so everytime we check whether we have the conditions
		to authorize the client or not */
	if (_userName.length() && _realName.length() && _nickName != "*" && _authorized)
		welcome();
}

void Client::nick(std::string &commandLine) { // 100% finished
	std::vector<std::string> args = getArgs(commandLine);
	// no params
	if (!args.size())
		logger.warn("431 " + _nickName + " :No nickname given");
	// check for forbidden chars
	for (std::string::iterator it = args[0].begin(); it != args[0].end(); it++) {
		if (std::string("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-[]\\`^{}_|").find(*it) == std::string::npos)
			logger.warn("432 " + _nickName + " " + args[0] + " :Erroneus nickname");
	}
	//check if nickname alredy exist
	bool alreadyExist = false;
	for (std::vector<Client *>::iterator it = irc_server.all_clients.begin(); it != irc_server.all_clients.end(); it++) {
		if ((*it)->_nickName == args[0]) {
			alreadyExist = true;
			break ;
		}
	}
	if (!alreadyExist) {
		std::string oldNick = _nickName;
		_nickName = args[0];
		logger.info("nickname to set: [" + args[0] + "]");
		if (oldNick != "*") {
			// broadcast nickname change to all clients
			for (std::vector<Client *>::iterator it = irc_server.all_clients.begin(); it != irc_server.all_clients.end(); it++)
				(*it)->send(":" + oldNick + " NICK " + _nickName + "\r\n");
		}
	}
	else
		logger.warn("433 " + _nickName + " " + args[0] + " :Nickname is already in use");
	/* we dont knoe which auth command will be executed the last so everytime we check whether we have the conditions
		to authorize the client or not */
	if (_userName.length() && _realName.length() && _nickName != "*" && _authorized)
		welcome();
}

void Client::user(std::string &commandLine) { // 100% finished
	std::vector<std::string> args = getArgs(commandLine);
	// check for params
	if (args.size() != 4)
		logger.warn("461 " + _nickName + " " + commandLine + " :Not enough parameters");
	std::string userName = args[0];
	std::string zero = args[1];
	std::string asterisk = args[2]; // asterisk = the character '*'
	std::string realName = args[3];
	// we dont need zero and asterisk, doenst matter what their value is
	_userName = userName;
	logger.verbose("username set to " + _userName);
	_realName = realName;
	logger.verbose("realname set to " + _realName);
	/* we dont knoe which auth command will be executed the last so everytime we check whether we have the conditions
		to authorize the client or not */
	if (_userName.length() && _realName.length() && _nickName != "*" && _authorized)
		welcome();
}

void Client::welcome(void) {
	// only welcome once
	if (_welcomed)
		return ;
	_welcomed = true;
	_registered = true;
	// sending welcome message is required so that users can do stuff in the server
	send("001 " + _nickName + " :Welcome to the Strix Internet Relay Chat Network " + _nickName + "\r\n");
	logger.info("User \033[1m" + _nickName + "\033[0m authorized.");
}

void Client::join(std::string &commandLine) {
	std::vector<std::string> args = getArgs(commandLine);
	// no params
	if (!args.size())
		logger.warn("461 " + _nickName + " JOIN :Not enough parameters");
	std::string channels = args[0];
	std::string passwords = (args.size() >= 2) ? args[1] : "";
	// loop used to go through all chained channels
	while (channels.find(",") != std::string::npos) {
		// we parse params from our args to join 1 channel
		std::string nextChannel = channels.substr(0, channels.find(","));
		channels.erase(0, nextChannel.length() + 1);
		std::string nextPassword = passwords.substr(0, passwords.find(","));
		passwords.erase(0, nextPassword.length() + 1);
		std::string nextCall = nextChannel + " " + nextPassword;
		// we call join again with our new params
		join(nextCall);
	}
	// inside try catch block so that we dont interrupt joining the remaining channels
	try {
		// channel names MUST not have white spaces
		if (channels.find(" ") != std::string::npos)
			logger.warn("476 " + _userName + " " + channels + " :Invalid channel name");
		// channel names MUST start with #
		if (channels[0] != '#') {
			logger.warn("403 " + _userName + " " + channels + " :No such channel");
	}
	} catch(std::exception &e) {
		send(e.what());
		return ;
	}
	// we check if channel already exist, if it does, add user to the channel, if not, create new channel
	for (std::vector<Channel *>::iterator it = irc_server.channels.begin(); it != irc_server.channels.end(); it++) {
		if ((*it)->_name == channels) {
			// i legit have no clue what am i doing at this point
			if (std::find((*it)->_members.begin(), (*it)->_members.end(), this) != (*it)->_members.end()) {
				// do nothing if member already part of the channel
				return ;
			}
			try {
				// check if pw match, if not, return wrong pw
				if (passwords != (*it)->_password)
					logger.warn("475 " + _nickName + " " + channels + " :Cannot join channel (+k) - bad key");
			} catch (std::exception &e) {
				send(e.what());
				return ;
			}
			// add user to existing channel members
			(*it)->_members.push_back(this);
			// inform everyone that user has joined
			(*it)->broadcast(":" + _nickName + "!~" + _userName + "@" + _IPAddress + " JOIN " + channels + "\r\n");
			// send list of all existing members in that channel
			send("353 " + _nickName + " = " + (*it)->_name + " :" + (*it)->getNames() + "\r\n");
			send("366 " + _nickName + " " + (*it)->_name + " :End of /NAMES list.\r\n");
			return ;
		}
	}
	// creating new channel since it doesnt exist already
	Channel *newChannel = new Channel(channels);
	irc_server.channels.push_back(newChannel);
	// add user to channel members + operators
	newChannel->_members.push_back(this);
	// add to operators since the first person to join/create a channel becomes its op
	newChannel->_operators.push_back(this);
	// inform everyone that user has joined
	newChannel->broadcast(":" + _nickName + "!~" + _userName + "@" + _IPAddress + " JOIN " + channels + "\r\n");
	// send list of all existing members in that channel
	send("353 " + _nickName + " = " + newChannel->_name + " :" + newChannel->getNames() + "\r\n");
	send("366 " + _nickName + " " + newChannel->_name + " :End of /NAMES list.\r\n");
}

void Client::privmsg(std::string &commandLine) {
	std::vector<std::string> args = getArgs(commandLine);
	if (args.size() <= 1)
		return ;
	std::string channel = args[0];
	std::string message = args[1];
	// handle multiple targets, like JOIN
	while (channel.find(",") != std::string::npos) {
		std::string nextChannel = channel.substr(0, channel.find(","));
		channel.erase(0, nextChannel.length() + 1);
		std::string nextCall = nextChannel + " :" + message;
		privmsg(nextCall);
	}
	bool onlyOP = false;
	// only operators will be able to see :O
	if (channel[0] == '@') {
		// to handle op only messages
		onlyOP = true;
		channel.erase(0, 1);
	}
	// channel stuff
	if (channel[0] == '#') {
		// find channel
		for (std::vector<Channel *>::iterator it = irc_server.channels.begin(); it != irc_server.channels.end(); it++) {
			// parse channel and message that we should broadcast
			if ((*it)->_name == channel) {
				// channel found
				if (std::find((*it)->_members.begin(), (*it)->_members.end(), this) != (*it)->_members.end())
					// user is member of that channel
					if (onlyOP) // broadcast to only channel operators instead of everyone
						(*it)->broadcastOP(":" + _nickName + "!~" + _userName + "@" + _IPAddress + " PRIVMSG " + channel + " :" + message + "\r\n", this);
					else
						(*it)->broadcast(":" + _nickName + "!~" + _userName + "@" + _IPAddress + " PRIVMSG " + channel + " :" + message + "\r\n", this);
				else
					send("404 " + _nickName + " " + channel + " :Cannot send to nick/channel\r\n");
				return ;
			}
		}
		logger.warn("403 " + _userName + " " + channel + " :No such channel");
	}
	// DM stuff
	else {
		// loop through all clients, if not found, return 401
		for (std::vector<Client *>::iterator it = irc_server.all_clients.begin(); it != irc_server.all_clients.end(); it++) {
			if ((*it)->_nickName == channel) {
				// :nickname PRIVMSG channel :message
				(*it)->send(":" + _nickName + " PRIVMSG " + channel + " :" + message + "\r\n");
				return ;
			}
		}
		logger.warn("401 " + _nickName + " " + channel + " :No such nick");
	}
}

void Client::part(std::string &commandLine) {
	std::vector<std::string> args = getArgs(commandLine);
	if (!args.size())
		logger.warn("461 " + _nickName + " JOIN :Not enough parameters");
	std::string channels = args[0];
	std::string leaveMessage = (args.size() >= 2) ? args[1] : "";
	// to handle multiple channels, like JOIN
	while (channels.find(",") != std::string::npos) {
		std::string nextChannel = channels.substr(0, channels.find(","));
		channels.erase(0, nextChannel.length() + 1);
		std::string nextCall = nextChannel + " " + leaveMessage;
		part(nextCall);
	}
	try {
		// channel names MUST not have white spaces
		if (channels.find(" ") != std::string::npos)
			logger.warn("476 " + _userName + " " + channels + " :Invalid channel name");
		// channel names MUST start with #
		if (channels[0] != '#')
			logger.warn("403 " + _userName + " " + channels + " :No such channel");
		for (std::vector<Channel *>::iterator it = irc_server.channels.begin(); it != irc_server.channels.end(); it++) {
			if ((*it)->_name == channels) {
				// i legit have no clue what am i doing at this point
				if (std::find((*it)->_members.begin(), (*it)->_members.end(), this) != (*it)->_members.end()) {
					// IS PART OF THE CHANNEL
					(*it)->broadcast(":" + _nickName + "!~" + _userName + "@" + _IPAddress + " PART " + channels + " " + leaveMessage + "\r\n");
					// the first function execution will execute the last chained channel, means this part will be executed the last, we use it to reset our leaveMessage cuz it is static!
					(*it)->_members.erase(std::find((*it)->_members.begin(), (*it)->_members.end(), this));
					if (!(*it)->_members.size()) {
						delete *it;
						irc_server.channels.erase(it);
					}
					return ;
				}
				logger.warn("403 " + _userName + " " + channels + " :No such channel");
			}
		}	
		logger.warn("403 " + _userName + " " + channels + " :No such channel");
	} catch(std::exception &e) {
		send(e.what());
		return ;
	}
}

void Client::kick(std::string &commandLine) {
	std::vector<std::string> args = getArgs(commandLine);
	static bool isFirst = true;
	if (args.size() < 2)
		logger.warn("461 " + _nickName + " :Not enough parameters");
	std::string channel = args[0];
	std::string users = args[1];
	// BTW users could be multiple users and could be only 1 user as well, its always 1 user while in recursive execution
	std::string reason = (args.size() >= 3) ? args[2] : "";
	// to handle multiple users, like JOIN
	while (users.find(",") != std::string::npos) {
		std::string nextUser = users.substr(0, users.find(","));
		users.erase(0, nextUser.length() + 1);
		std::string nextCall = channel + " " + nextUser + " " + reason;
		kick(nextCall);
	}
	for (std::vector<Channel *>::iterator it = irc_server.channels.begin(); it != irc_server.channels.end(); it++) {
		if ((*it)->_name == channel) {
			// channel found
			// now we check if user is in channel
			if (std::find((*it)->_members.begin(), (*it)->_members.end(), this) == (*it)->_members.end())
				logger.warn("442 " + _nickName + " " + channel + " :You're not on that channel");			
			// now we check if user has privilege
			if (std::find((*it)->_operators.begin(), (*it)->_operators.end(), this) == (*it)->_operators.end())
				logger.warn("482 " + _nickName + " " + channel + " :You're not a channel operator");
			for (std::vector<Client *>::iterator _it = (*it)->_operators.begin(); _it != (*it)->_operators.end(); _it++) {
				if ((*_it)->_nickName == users) {
					// operator found
					(*it)->_operators.erase(_it);
					break ;
				}
			}
			for (std::vector<Client *>::iterator _it = (*it)->_members.begin(); _it != (*it)->_members.end(); _it++) {
				if ((*_it)->_nickName == users) {
					// user found
					// broadcast kick message to all members (we let everyone know users mcha y9awed)
					(*it)->broadcast(":" + _nickName + "!~" + _userName + "@" + _IPAddress + " KICK " + channel + " " + users + " :" + reason + "\r\n");
					(*it)->_members.erase(_it);
					if (!(*it)->_members.size()) {
						// channel empty
						delete (*it);
						irc_server.channels.erase(it);
					}
					return ;
				}
			}
			logger.warn("401 " + _nickName + " " + users + " :No such nick/channel");
		}
	}
	logger.warn("403 " + _nickName + " " + channel + " :No such channel");
}

void Client::mode(std::string &commandLine) {
}

void Client::quit(std::string &commandLine) {
	if (commandLine[0] == ':')
		commandLine.erase(0, 1);
	_leaveMessage = commandLine;
	send("ERROR :Closing Link: " + _IPAddress + " (Client Quit)\r\n");
	_keepAlive = false;
}

// Error(474): #e Cannot join channel (+b) - you are banned bruhh (i literally got banned from a public IRC server channel cuz i was spam testing commands lmao)
// brojola hadshi tl3li frassi hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhelpmepleasehhhhhhhhhhhhhhhhhhhhhh
