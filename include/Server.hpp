#pragma once

#include <cstdint>
#include <sys/poll.h>
#include <vector>
#include "meta.hpp"



class Server
{
public:
	Server();
	Server(const Server &) = default;
	Server &operator=(const Server &) = default;
	~Server() = default;


	const std::vector<pollfd>& getClients();


private:
	std::vector<pollfd> _listenFds;
	std::vector<pollfd> _clientFds;


	int _socketCreate();
	int _socketBind(int &fd, uint16_t port);
	
};

