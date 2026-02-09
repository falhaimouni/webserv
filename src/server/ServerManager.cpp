/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 22:50:12 by marvin            #+#    #+#             */
/*   Updated: 2026/01/25 22:50:12 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerManager.hpp"
#include "Logger.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>     // close
#include <cstring>      // memset
#include <sstream>      // ostringstream
#include <fcntl.h>


ServerManager::ServerManager() : _listenFd(-1) {}

ServerManager::~ServerManager() {
    if (_listenFd != -1)
        close(_listenFd);
}

int ServerManager::getListenFd() const {
    return _listenFd;
}

bool ServerManager::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        Logger::error("fcntl(F_GETFL) failed");
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        Logger::error("fcntl(F_SETFL) failed");
        return false;
    }

    Logger::info(std::string("setNonBlocking() OK on fd=") + Logger::toStringInt(fd));
    return true;
}
bool ServerManager::start(int port)
{
    Logger::info("ServerManager::start called");
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    //int listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
    //std::cout << "Listen FD: " << listenfd2 << std::endl;
    if (_listenFd < 0)
    {
        Logger::error("socket() failed");
        return false;
    }
    // std::cout << "Listen FD: " << _listenFd << std::endl;
    Logger::info(std::string("socket() OK fd=") + Logger::toStringInt(_listenFd));

    int opt = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        Logger::error("setsockopt(SO_REUSEADDR) failed");
        close(_listenFd);
        _listenFd = -1;
        return false;
    }
    Logger::info("setsockopt(SO_REUSEADDR) OK");

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        Logger::error("bind() failed (maybe port is busy)");
        close(_listenFd);
        _listenFd = -1;
        return false;
    }
    Logger::info(std::string("bind() OK on port ") + Logger::toStringInt(port));

    if (listen(_listenFd, 128) < 0)
    {
        Logger::error("listen() failed");
        close(_listenFd);
        _listenFd = -1;
        return false;
    }
    Logger::info(std::string("listen() OK - Listening on port ") + Logger::toStringInt(port));
    if (!setNonBlocking(_listenFd))
    {
        Logger::error("Failed to set listening socket non-blocking");
        close(_listenFd);
        _listenFd = -1;
        return false;
    }
    Logger::info("Listening socket set to non-blocking");
    return true;
}

int ServerManager::acceptClient()
{
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    std::memset(&clientAddr, 0, sizeof(clientAddr));

    int clientFd = accept(_listenFd,
                          reinterpret_cast<sockaddr*>(&clientAddr),
                          &clientLen);

    if (clientFd < 0)
        return -1;

    if (!setNonBlocking(clientFd))
    {
        Logger::error("Failed to set client socket non-blocking");
        close(clientFd);
        return -1;
    }

    Logger::info(std::string("Accepted client fd=") + Logger::toStringInt(clientFd));
    return clientFd;
}
