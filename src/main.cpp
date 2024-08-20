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
#include <vector>

#define LOG_ERROR(x) std::cerr << "[ERROR] : " << x << std::endl;
#define LOG(x) std::cout << "[INFO] : " << x << std::endl;

#define POLL_TIMEOUT 100
#define LISTEN_BACKLOG 8


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

	if (listen(serverFd, LISTEN_BACKLOG) == -1)
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
		// read up on EWOULDBLOCK
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

int main()
{
	int serverFd;
	if (!setup(8080, serverFd))
	{
		return 1;
	}

	std::vector<pollfd> pollFds;

	pollFds.push_back({serverFd, POLLIN, 0});


	while (1)
	{
		int nReady = poll(pollFds.data(), pollFds.size(), POLL_TIMEOUT);
		if (nReady == -1)
		{
			LOG_ERROR("Failed polling: " << strerror(errno));
			return 0;
		}
		LOG("nReady: " << nReady);


		for (size_t i = 0; i < pollFds.size(); i++)
		{
			pollfd &pfd = pollFds[i];
			LOG("checking sock fd: " << pfd.fd);

			// check if current fd is listening socket.
			int val;
			socklen_t len = sizeof(val);
			getsockopt(pfd.fd, SOL_SOCKET, SO_ACCEPTCONN, &val, &len);
			if (val && pfd.revents)
			{
				// Handle listening socket.
				LOG("event on listen socket " << pfd.fd)
				int clientFd = acceptConnections(serverFd);
				pollFds.push_back({clientFd, POLLIN | POLLOUT, 0});
			}

			else if (pfd.revents)
			{
				if (pfd.revents & POLLIN)
				{
					LOG("fd: " << pfd.fd << " POLLIN");
				}
				if (pfd.revents & POLLOUT)
				{
					LOG("fd: " << pfd.fd << " POLLOUT");
					// std::string msg = "client on fd: " + std::to_string(pfd.fd) + "\n";
					// send(pfd.fd, msg.c_str(), msg.length() * sizeof(char), 0);
				}
			}
		}
	}





	close(serverFd);
	for (pollfd pfd : pollFds)
	{
		close(pfd.fd);
	}

	LOG("Test");

	return 0;
}

