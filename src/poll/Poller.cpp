/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Poller.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 22:48:12 by marvin            #+#    #+#             */
/*   Updated: 2026/01/25 22:48:12 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Poller.hpp"
#include "Logger.hpp"

Poller::Poller() {}
Poller::~Poller() {}

std::vector<pollfd>& Poller::getPollFds()
{
    return _pfds;
}

void Poller::addFd(int fd, short events)
{
    pollfd p;
    p.fd = fd;
    p.events = events;
    p.revents = 0;
    _pfds.push_back(p);
    Logger::info(std::string("Poller add fd=") + Logger::toStringInt(fd));
}

void Poller::removeFd(int fd)
{
    for (size_t i = 0; i < _pfds.size(); i++)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds.erase(_pfds.begin() + i);
            Logger::info(std::string("Poller remove fd=") + Logger::toStringInt(fd));
            return;
        }
    }
}

void Poller::setEvents(int fd, short events)
{
    for (size_t i = 0; i < _pfds.size(); i++)
    {
        if (_pfds[i].fd == fd)
        {
            _pfds[i].events = events;
            return;
        }
    }
}

int Poller::wait(int timeoutMs)
{
    if (_pfds.empty())
        return 0;

    int ret = poll(&_pfds[0], _pfds.size(), timeoutMs);
    if (ret < 0)
        Logger::error("poll() failed");
    return ret;
}