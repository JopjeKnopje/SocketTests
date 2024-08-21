#pragma once

#define POLL_TIMEOUT 100
#define LISTEN_BACKLOG 8

#define LOG_ERROR(x) std::cerr << "[ERROR] : " << x << std::endl;
#define LOG(x) std::cout << "[INFO] : " << x << std::endl;
