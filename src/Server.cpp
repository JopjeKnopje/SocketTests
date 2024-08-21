#include "Server.hpp"
#include "meta.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <set>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>


static bool echo(int fd)
{
	char buffer[1024];
	const std::string bye_str = "Cya!\n";
	bzero(&buffer, sizeof(buffer));
	// read up on EWOULDBLOCK
	recv(fd, buffer, sizeof(buffer), 0);
	LOG("Received: [" << buffer << "]");

	if (!buffer[0] || !std::strcmp(buffer, "yup\r\n"))
	{
		send(fd, bye_str.c_str(), bye_str.length() * sizeof(char), 0);
		return false;
	}
	send(fd, buffer, sizeof(buffer), 0);
	return true;
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
	for (size_t i = 0; i < _fds.size(); i++)
	{
		pollfd &pfd = _fds[i];
		LOG("checking fd: " << pfd.fd);

		// if current fd is a listener
		if (pfd.revents && std::count(getListenFds().begin(), getListenFds().end(), pfd.fd))
		{
			_fds.push_back({_socketAccept(pfd.fd), POLLIN | POLLOUT, 0});
		}
		else if (pfd.revents & POLLIN)
		{
			LOG("fd: " << pfd.fd << " POLLIN");
			if (!echo(pfd.fd))
			{
				close(pfd.fd);
				_fds.erase(_fds.begin() + i);
			}
		}
		else if (pfd.revents & POLLOUT)
		{
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

