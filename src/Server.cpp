#include "Server.hpp"
#include <cstdlib>
#include <map>
#include <sys/wait.h>
#include <strings.h>
#include <unistd.h>
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
#include <utility>
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


int pipe_fds[2];
std::pair<int, int> cgi_start()
{

	::pipe(pipe_fds);

	int pid = fork();
	if (pid == -1)
	{
		LOG_ERROR("Fork failed");
	}
	if (pid == 0)
	{
		close(pipe_fds[0]);
		LOG("Child spawned");
		if (dup2(pipe_fds[1], STDOUT_FILENO) == -1)
		{
			LOG_ERROR("dup2 failed");
		}
		close(pipe_fds[1]);


		// char * const argv[] = {
		// 	"sleep",
		// 	"5",
		// 	NULL,
		// };

		char * const argv[] = {
			"ls",
			"-lsa",
			NULL,
		};

		if (execvp(argv[0], argv) == -1)
			LOG_ERROR("execvp failed");

		exit(123);
	}

	close(pipe_fds[1]);


	return {pipe_fds[0], pid};
}

std::string cgi_wait(int pid, int pipe_fd_read)
{

	const size_t buf_size = 123;
	char buf[buf_size];
	bzero(&buf, buf_size);

	int32_t status;
	LOG("waiting on pid: " << pid);
	::waitpid(pid, &status, 0);

	read(pipe_fd_read, buf, buf_size - 1);
	close(pipe_fd_read);
	LOG("read from pipe: " << buf);
	
	LOG("exit code: " << WEXITSTATUS(status));
	return buf;
}



static std::pair<int, int> response_start(int fd)
{
	const size_t buf_size = 1024;
	char buffer[buf_size];
	const std::string bye_str = "Cya!\n";
	bzero(&buffer, sizeof(buffer));
	// read up on EWOULDBLOCK
	recv(fd, buffer, buf_size, 0);
	LOG("Received: [" << buffer << "]");



	return cgi_start();

	// return and contintue to poll shit
}

static void reponse_wait(int pid, int pipe_fd_read, int client_fd)
{

	// jump back in and finish building response
	cgi_wait(pid, pipe_fd_read);

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



	send(client_fd, s.c_str(), s.length(), 0);

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
	// pipe_fd, <cgi_pid, client_fd>
	std::map<int, std::pair<int, pollfd>> shit;

	for (size_t i = 0; i < _fds.size(); i++)
	{
		pollfd &pfd = _fds[i];
		LOG("checking fd: " << pfd.fd);

		// check if pfd.fd is in map instead of this count bs with a second vector.

		if (pfd.revents && shit.find(pfd.fd) != shit.end())
		{
			LOG("pfd.fd : " << pfd.fd << " is a pipe");
			// const pollfd client_pfd = shit[pfd.fd].second;
			// const int cgi_pid = shit[pfd.fd].first;
			// const int pipe_fd = pfd.fd;
			//
			// reponse_wait(cgi_pid, pipe_fd, client_pfd.fd);
			// shit.erase(shit.find(pipe_fd));
			//
			// close(client_pfd.fd);

			// _fds.erase(std::find(_fds.begin(), _fds.end(), client_pfd));
			// shit.erase(shit.find(pfd.fd));
		}


		else if (pfd.revents && std::count(getListenFds().begin(), getListenFds().end(), pfd.fd))
		{
			// if new connection
			// vec add new ClientFd
			// connections.add new connection
			_fds.push_back({_socketAccept(pfd.fd), POLLIN | POLLOUT, 0});
		}
		else if (pfd.revents & POLLIN)
		{
			LOG("fd: " << pfd.fd << " POLLIN");

			std::pair<int, int> pipefd_pid = response_start(pfd.fd);

			pollfd newpfd {pipefd_pid.first, POLLIN, 0};

			_fds.push_back(newpfd);
			shit[pipefd_pid.first] = {pipefd_pid.second, newpfd};

			

			// close(pfd.fd);
			// _fds.erase(_fds.begin() + i);
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

