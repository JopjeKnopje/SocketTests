#include "Server.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <set>
#include <sys/socket.h>
#include <unistd.h>


static bool setup(uint16_t port)
{
	int serverFd = socket(AF_INET, SOCK_STREAM, 0);

	if (serverFd == -1)
	{
		LOG_ERROR(strerror(errno));
		return false;
	}
	else
	{
		LOG("Created socket with fd: " << serverFd);
	}


	int enable = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)))
	{
		LOG_ERROR(strerror(errno));
		close(serverFd);
		return false;
	}
	
	return true;

}

int Server::_socketBind(int &fd, uint16_t port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;


	if (bind(fd, (sockaddr *) &addr, sizeof(sockaddr_in)) == -1)
	{
		LOG_ERROR("Failed binding to port: " << port << ", " << strerror(errno));
		close(fd);
		return -1;
	}
	else
	{
		LOG("Bound to port: " << port);
	}

	if (listen(fd, LISTEN_BACKLOG) == -1)
	{
		LOG_ERROR("Listen failed, " << strerror(errno));
		close(fd);
		return -1;
	}
	return true;
}

Server::Server()
{
}

const std::vector<pollfd>& Server::getClients()
{
	return _clientFds;
}

