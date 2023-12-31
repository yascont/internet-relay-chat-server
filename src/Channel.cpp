#include "Channel.hpp"

// vars to reset: _password="", _inviteOnly=false
Channel::Channel(std::string &name): _name(name), _password(""), _topic(""), _opOnlyTopic(false), _inviteOnly(false), _limit(0) { // channel default settings
}

// broadcast to all members
void Channel::broadcast(std::string message) {
	for (std::vector<Client *>::iterator it = _members.begin(); it != _members.end(); it++)
		(*it)->send(message);
}

// broadcast to all members except the broadcaster
void Channel::broadcast(std::string message, Client *broadcaster) {
	for (std::vector<Client *>::iterator it = _members.begin(); it != _members.end(); it++)
		if (*it != broadcaster)
			(*it)->send(message);
}

// broadcast to operators except the broadcaster
void Channel::broadcastOP(std::string message, Client *broadcaster) {
	for (std::vector<Client *>::iterator it = _operators.begin(); it != _operators.end(); it++)
		if (*it != broadcaster)
			(*it)->send(message);
}

// return _members.map(e => _operators.includes(e) ? "@" + e : e).join(" ");
std::string Channel::getNames(void) {
	std::string names = "";
	for (std::vector<Client *>::iterator it = _members.begin(); it != _members.end(); it++)
		names += std::string(names.length() ? " " : "") + ((std::find(_operators.begin(), _operators.end(), *it) !=  _operators.end()) ? "@" : "") + (*it)->_nickName; // XD?
	return names;
}

std::string Channel::getModes(void) {
	std::string modes = "+";
	std::string args = "";
	if (_inviteOnly)
		modes += 'i';
	if (_password != "") {
		modes += 'k';
		args += " " + _password;
	}
	if (_limit) {
		modes += 'l';
		args += " " + std::to_string(_limit);
	}
	if (_opOnlyTopic)
		modes += 't';
	return (modes + args);
}