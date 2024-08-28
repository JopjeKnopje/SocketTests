#include "Server.hpp"
#include "meta.hpp"
#include <cstddef>
#include <iostream>
#include <cstring>


int main()
{
	std::vector<Server> servers;

	servers.push_back({{8080, 8081}});
	servers.push_back({{9090, 9091}});


	bool printReady = true;

	while (1)
	{
		for (size_t i = 0; i < servers.size(); i++)
		{
			Server &s = servers[i];
			int nReady = poll(s.getFds().data(), s.getFds().size(), POLL_TIMEOUT);
			if (nReady == -1)
			{
				LOG_ERROR("Failed polling: " << strerror(errno));
				return 0;
			}
			else if (printReady && !nReady)
			{
				LOG("nReady: " << nReady);
				printReady = false;
			}
			else if (nReady)
			{
				LOG("nReady: " << nReady);
				printReady = true;
				s.handleEvents();
			}
		}
	}
	return 0;
}
