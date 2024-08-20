#include <algorithm>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory_resource>
#include <string>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <type_traits>
#include <unistd.h>
#include <poll.h>

#define LOG_ERROR(x) std::cerr << "[ERROR] : " << x << std::endl;
#define LOG(x) std::cout << "[INFO] : " << x << std::endl;

#define POLL_TIMEOUT 100


bool setup(const uint16_t port, int &serverFd)
{
	serverFd = socket(AF_INET, SOCK_STREAM, 0);

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
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;


	if (bind(serverFd, (sockaddr *) &addr, sizeof(sockaddr_in)) == -1)
	{
		LOG_ERROR("Failed binding to port: " << port << ", " << strerror(errno));
		close(serverFd);
		return false;
	}
	else
	{
		LOG("Bound to port: " << port);
	}

	if (listen(serverFd, 1) == -1)
	{
		LOG_ERROR("Listen failed, " << strerror(errno));
		close(serverFd);
		return false;
	}



	return true;
}


int acceptConnections(int serverFd)
{
	// accept is a blocking function
	int clientFd = accept(serverFd, nullptr, nullptr);
	if (clientFd == -1)
	{
		LOG_ERROR("Failed accepting client on fd: " << serverFd << ", " << strerror(errno));
		close(serverFd);
		return false;
	}
	else
	{
		LOG("Accepted client on fd: " << serverFd << " with clientFd " << clientFd);
	}

	return clientFd;
}


void handleConnection(int fd)
{
	char buffer[1024];
	const std::string bye_str = "Cya!\n";
	while (1)
	{
		bzero(&buffer, sizeof(buffer));
		recv(fd, buffer, sizeof(buffer), 0);
		LOG("Received: [" << buffer << "]");

		if (!buffer[0] || !std::strcmp(buffer, "yup\r\n"))
		{
			send(fd, bye_str.c_str(), bye_str.length() * sizeof(char), 0);
			break;
		}
		send(fd, buffer, sizeof(buffer), 0);
	}

}

int main ()
{
	int serverFd, clientFd;
	if (!setup(8080, serverFd))
	{
		return 1;
	}



	while (1)
	{
		// acceptConnections(serverFd);

		// check for IO on other conns.

		pollfd pfd;
		pfd.fd = serverFd;
		pfd.events = POLLIN | POLLOUT;
		pfd.revents = 0;

		int nReady = poll(&pfd, 1, POLL_TIMEOUT);
		if (nReady == -1)
		{
			LOG_ERROR("Failed polling: " << strerror(errno));
			return 0;
		}
		LOG("nReady: " << nReady);
		if (pfd.revents & POLLIN)
		{
			LOG("Ready: POLLIN");
			int clientFd = acceptConnections(serverFd);
		}
		if (pfd.revents & POLLOUT)
			LOG("Ready: POLLOUT");


	}





	close(serverFd);
	close(clientFd);

	LOG("Test");

	return 0;
}
