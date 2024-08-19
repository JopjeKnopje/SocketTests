#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define LOG_ERROR(x) std::cerr << "[ERROR] : " << x << std::endl;
#define LOG(x) std::cout << "[INFO] : " << x << std::endl;


int main ()
{
	std::cout << "message" << std::endl;


	int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1)
	{
		LOG_ERROR(strerror(errno));
	}
	else
	{
		LOG("Created socket with fd: " << serverFd);
	}

	
	const uint16_t port = 8084;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;


	if (bind(serverFd, (sockaddr *) &addr, sizeof(sockaddr_in)) == -1)
	{
		LOG_ERROR("Failed binding to port: " << port << ", " << strerror(errno));
	}
	else
	{
		LOG("Bound to port: " << port);
	}


	if (listen(serverFd, 1) == -1)
	{
		LOG_ERROR("Listen failed, " << strerror(errno));
	}
	else
	{
		LOG("Done listening");
	}


	// accept is a blocking function
	int clientFd = accept(serverFd, nullptr, nullptr);
	if (clientFd == -1)
	{
		LOG_ERROR("Failed accepting client on fd: " << serverFd << ", " << strerror(errno));
	}
	else
	{
		LOG("Accepted client on fd: " << serverFd << " with clientFd " << clientFd);
	}


	char buffer[1024];
	bzero(&buffer, sizeof(buffer));
	recv(clientFd, buffer, sizeof(buffer), 0);
	LOG("Received: " << buffer);


	close(serverFd);
	close(clientFd);

	LOG("Test");

	return 0;
}
