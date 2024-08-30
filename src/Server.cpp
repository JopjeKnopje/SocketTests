#include "Server.hpp"
#include "meta.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>


void replace_first(
    std::string& s,
    std::string const& toReplace,
    std::string const& replaceWith
) {
    std::size_t pos = s.find(toReplace);
    if (pos == std::string::npos) return;
    s.replace(pos, toReplace.length(), replaceWith);
}

static bool echo(int fd)
{
	const size_t buf_size = 1024;
	char buffer[buf_size];
	const std::string bye_str = "Cya!\n";
	bzero(&buffer, sizeof(buffer));
	// read up on EWOULDBLOCK
	recv(fd, buffer, buf_size, 0);
	LOG("Received: [" << buffer << "]");


	std::string s = 
	"HTTP/1.1 200 OK\r\n"
	"Server: nginx/1.27.1\r\n"
	"Date: Thu, 29 Aug 2024 13:02:31 GMT\r\n"
	"Content-Type: text/html\r\n"
	"Content-Length: 615\r\n"
	"Last-Modified: Mon, 12 Aug 2024 14:21:01 GMT\r\n"
	// "Connection: closed\r\n"
	// "ETag: \\"66ba1a4d-267"\\"
	"Accept-Ranges: bytes\r\n";

	s += "\r\n<h3> Fakka strijders </h3>\r\n";

	// if (!s[0] || !std::strcmp(s.c_str(), "yup\r\n"))
	// {
	// 	send(fd, bye_str.c_str(), bye_str.length() * sizeof(char), 0);
	// 	return false;
	// }
	if (!s[0])
	{
		return false;
	}
	send(fd, s.c_str(), s.length(), 0);


	// NOTE according to the http docs we can use the same connection for multiple requests.
	// Therefore we dont HAVE to close the connection after sending this response.
	// However in our current setup (in which we will send only 1 response this will work)
	// https://developer.mozilla.org/en-US/docs/Web/HTTP/Overview

	return false;
}



Server::Server(uint16_t port) : Server(std::vector<uint16_t>({port}))
{

}

Server::Server(std::vector<uint16_t> ports)
{
	for (uint16_t p : ports)
	{
		int fd = _socketCreate();

		if (fd == -1)
		{
			UNIMPLEMENTED("handle _socketCreate failed");
		}

		if (!_socketBind(fd, p))
		{
			UNIMPLEMENTED("handle _socketBind failed");
		}

		_listenFds.push_back(fd);
		_fds.push_back({fd, POLLIN, 0});
	}
}

void Server::handleEvents()
{
	// loop over connections
	// vec <fd> 

	for (size_t i = 0; i < _fds.size(); i++)
	{
		pollfd &pfd = _fds[i];
		LOG("checking fd: " << pfd.fd);

		// if current fd is a listener
		// if current fd is ListenFd
		if (pfd.revents && std::count(getListenFds().begin(), getListenFds().end(), pfd.fd))
		{
			// if new connection
			// vec add new ClientFd
			// connections.add new connection
			_fds.push_back({_socketAccept(pfd.fd), POLLIN | POLLOUT, 0});
		}
		else if (pfd.revents & POLLIN)
		{
			// connection.gethttphandler.appendToBuf(connection.data)
			LOG("fd: " << pfd.fd << " POLLIN");
			if (!echo(pfd.fd))
			{
				close(pfd.fd);
				_fds.erase(_fds.begin() + i);
			}
		}
		else if (pfd.revents & POLLOUT)
		{
			// connection.httphandler.response.ready?
			// connection.send
			LOG("fd: " << pfd.fd << " POLLOUT");
		}
	}
}



const std::vector<int>& Server::getListenFds() const
{
	return _listenFds;
}

std::vector<pollfd>& Server::getFds()
{
	return _fds;
}



int Server::_socketCreate()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd == -1)
	{
		LOG_ERROR(strerror(errno));
		return fd;
	}
	else
		LOG("Created socket with fd: " << fd);

	int enable = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
	{
		LOG_ERROR(strerror(errno));
		close(fd);
	}
	return fd;
}

bool Server::_socketBind(int fd, uint16_t port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(fd, (sockaddr *) &addr, sizeof(sockaddr_in)) == -1)
	{
		LOG_ERROR("Failed binding to port: " << port << ", " << strerror(errno));
		close(fd);
		return false;
	}
	else
		LOG("Bound to port: " << port);

	if (listen(fd, LISTEN_BACKLOG) == -1)
	{
		LOG_ERROR("Listen failed, " << strerror(errno));
		close(fd);
		return false;
	}
	return true;
}

int Server::_socketAccept(int fd)
{
	int clientFd = accept(fd, nullptr, nullptr);
	if (clientFd == -1)
	{
		LOG_ERROR("Failed accepting client on fd: " << fd << ", " << strerror(errno));
		close(fd);
		return -1;
	}
	else
	{
		LOG("Accepted new client on listening socket fd: " << fd << " with clientFd " << clientFd);
	}

	return clientFd;
}

