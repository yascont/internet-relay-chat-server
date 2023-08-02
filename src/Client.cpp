#include "Client.hpp"
#include <map>
#include <Logger.hpp>

// I USE logger.warn TO SEND ERROR MESSAGES BACK TO THE CLIENT CHECK Client.hpp/Logger.hpp

// these are the commands
Client::Client(int &client_fd): _clientFd(client_fd), _nickName("*"), _authorized(false), _registered(false), _keepAlive(true), logger(client_fd) {
	_commands.insert(std::make_pair("USER", &Client::user));
	_commands.insert(std::make_pair("PASS", &Client::pass));
	_commands.insert(std::make_pair("NICK", &Client::nick));
}

int Client::getFd(void) {
	return this->_clientFd;
}

// this will send a message to the client
void Client::send(std::string msg) {
	::send(_clientFd, msg.c_str(), msg.length(), 0);
}

void Client::execute(std::string commandLine) {
	logger.debug("Executing command line: [" + commandLine + "]");
	// parse the command, first word before a space
	std::string command = commandLine.substr(0, commandLine.find(" "));
	// delete it, now we got command that contains the command and commandLine with args
	commandLine.erase(0, command.length());
	logger.verbose("command: [" + command + "]");
	// skip skip
	while (commandLine[0] == ' ')
		commandLine.erase(0, 1);
	logger.verbose("args: [" + commandLine + "]");
	try {
		// user is not allowed to auth after authenticating for the first time
		if ((command == "PASS" || command == "USER") && _registered)
			logger.warn("462 " + _nickName + " :You are already connected and cannot handshake again");
		// any command other than PASS, NICK, USER is not allowed if not authorized yet
		if ((command != "PASS" && command != "USER" && command != "NICK") && !_registered) {
			logger.warn("464 " + _nickName + " :You have not registered");
		}
		// if command exist, execute, otherwise, unknown command
		if (_commands[command]) {
			(this->*_commands[command])(commandLine);
		}
		else {
			logger.warn("421 " + _nickName + " " + command + " :Unknown command");
		}
	} catch (std::exception &e) {
			// any thrown error will be sent back to the client
			send(e.what());
			// disconnect user if its dead, not useful yet
			if (!_keepAlive) {
				close(_clientFd);
			}
	}
}
