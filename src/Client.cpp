/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 12:15:35 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/14 18:29:21 by jordanleal       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Client.h>
#include <master.h>

Client::Client()
	: fd(-1),
	op(false),
	passAccepted(false),
	hasNickname(false),
	hasUsername(false),
	registered(false)
{}

Client::~Client() {}

int Client::getFd() const { return fd; }
const std::string &Client::getIp() const { return ip; }
bool Client::getOp() const { return op; }

void Client::setFd(int fd) { this->fd = fd; }
void Client::setOp(bool op) { this->op = op; }
void Client::setIp(const std::string &ip) { this->ip = ip; }

const std::string &Client::getNickname() const { return nickname; }
const std::string &Client::getUsername() const { return username; }
const std::string &Client::getRealName() const { return realname; }

bool Client::isPassAccepted() const { return passAccepted; }
bool Client::isRegistered() const { return registered; }

void Client::setNick(const std::string &nickname) {
	this->nickname = nickname;
	updateRegistration();
}

void Client::setUsername(const std::string &username) {
	this->username = username;
	updateRegistration();
}

void Client::setRealname(const std::string &realname) {
	this->realname = realname;
	updateRegistration();
}

void Client::setPassAccepted(bool value) {
	this->passAccepted = value;
	updateRegistration();
}

bool Client::appendToReadBuffer(const std::string &data) {
	if (data.empty())
		return (true);
	this->readBuffer += data;

	std::string::size_type lineStart = 0;
	std::string::size_type newline = 0;

	while ((newline = readBuffer.find('\n', lineStart)) != std::string::npos)
	{
		if (newline - lineStart + 1 > LINE_LEN_BUF_MAX)
			return (false);
		lineStart = newline + 1;
	}
	if (readBuffer.size() - lineStart > LINE_LEN_BUF_MAX)
		return (false);
	return (true);
}

bool Client::hasCompleteLine() const {
	return (this->readBuffer.find('\n') != std::string::npos);
}

std::string Client::popLine() {
	std::string::size_type end;
	std::string::size_type lineEnd;
	std::string line;
	
	end = readBuffer.find('\n');
	if (end == std::string::npos)
		return ("");
	
	if (end + 1 > LINE_LEN_BUF_MAX)
		throw std::runtime_error("IRC line too long");
	
	lineEnd = end;
	if (lineEnd > 0 && readBuffer[lineEnd - 1] == '\r')
		--lineEnd;
	line = readBuffer.substr(0, lineEnd);
	readBuffer.erase(0, end + 1);
	
	return (line);
}

void Client::queueMessage(const std::string &message) {
	if (message.empty())
		return;
	sendQueue.push_back(message);
}

bool Client::hasPendingOutput() const { return (!sendQueue.empty()); }

std::string &Client::frontOutput() { return (sendQueue.front()); }

void Client::popOutput() { sendQueue.pop_front(); }

void Client::updateRegistration() {
	hasNickname = !nickname.empty();
	hasUsername = !username.empty();
	registered = (passAccepted && hasNickname && hasUsername);
}
