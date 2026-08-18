/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 12:15:35 by htrindad          #+#    #+#             */
/*   Updated: 2026/07/01 20:35:15 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Client.h>
#include <master.h>

Client::Client()
	: fd(-1),
	port(0),
	queuedBytes(0),
	passAccepted(false),
	hasNickname(false),
	hasUsername(false),
	registered(false)
{}

Client::~Client() {}

int					Client::getFd() const { return fd; }
const std::string	&Client::getIp() const { return ip; }
int					Client::getPort() const { return port; }

void Client::setFd(int fd) { this->fd = fd; }
void Client::setIp(const std::string &ip) { this->ip = ip; }
void Client::setPort(int port) { this->port = port; }

const std::string &Client::getNickname() const { return nickname; }
const std::string &Client::getUsername() const { return username; }
const std::string &Client::getRealName() const { return realname; }

bool Client::isPassAccepted() const { return passAccepted; }
bool Client::isRegistered() const { return registered; }

void Client::setNick(const std::string &nickname)
{
	this->nickname = nickname;
	hasNickname = !nickname.empty();
}

void Client::setUsername(const std::string &username)
{
	this->username = username;
	hasUsername = !username.empty();
}

void Client::setRealname(const std::string &realname)
{
	this->realname = realname;
}

void Client::setPassAccepted(bool value)
{
	this->passAccepted = value;
}

bool Client::tryMarkRegistered()
{
	if (registered)
		return false;
	if (!passAccepted || !hasNickname || !hasUsername)
		return false;
	registered = true;
	return true;
}

bool Client::appendToReadBuffer(const std::string &data)
{
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

bool Client::hasCompleteLine() const
{
	return (this->readBuffer.find('\n') != std::string::npos);
}

std::string Client::popLine()
{
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

bool Client::queueMessage(const std::string &message)
{
	if (message.empty())
		return (true);
	if (message.size() > MAX_QUEUED_BYTES)
		return (false);
	if (queuedBytes > MAX_QUEUED_BYTES - message.size())
		return (false);

	sendQueue.push_back(message);
	queuedBytes += message.size();
	return (true);
}

void	Client::consumeOutput(size_t bytes)
{
	if (sendQueue.empty() || bytes == 0)
		return;

	std::string &msg = sendQueue.front();

	if (bytes >= msg.size())
	{
		queuedBytes -= msg.size();
		sendQueue.pop_front();
	}
	else
	{
		msg.erase(0, bytes);
		queuedBytes -= bytes;
	}
}

bool		Client::hasPendingOutput() const	{ return (!sendQueue.empty()); }

std::string	&Client::frontOutput()				{ return (sendQueue.front()); }

void	Client::updateRegistration()
{
	hasNickname = !nickname.empty();
	hasUsername = !username.empty();
}
