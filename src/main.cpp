#include "Server.hpp"
#include "meta.hpp"
#include <cstddef>
#include <iostream>
#include <cstring>


int main()
{
	std::vector<Server> servers;

	servers.push_back({{8080, 8081}});


	bool state = true;

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
			else if (state && !nReady)
			{
				LOG("nReady: " << nReady);
				state = false;
			}
			else if (nReady)
			{
				LOG("nReady: " << nReady);
				state = true;
				s.handleEvents();
			}


		}
	}
	return 0;
}
