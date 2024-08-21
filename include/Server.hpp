#pragma once

#include <cstdint>
#include <sys/poll.h>
#include <vector>



class Server
{
public:
	Server();
	Server(const Server &) = default;
	Server &operator=(const Server &) = default;
	~Server() = default;


	Server(uint16_t port);
	Server(std::vector<uint16_t> ports);


	void handleEvents();

	const std::vector<int>& getListenFds() const;
	std::vector<pollfd>& getFds();

private:
	std::vector<pollfd> _fds;
	std::vector<int> _listenFds;

	static int _socketCreate();
	static bool _socketBind(int fd, uint16_t port);
	static int _socketAccept(int fd);
};
