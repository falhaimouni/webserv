#include "ConfigParser.hpp"
#include "ServerManager.hpp"
#include "Poller.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include <map>
#include <vector>
#include <ctime>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        Logger::error("Usage: ./webserv <config_file>");
        return 1;
    }

    ConfigParser parser;
    if (!parser.parse(argv[1]))
        return 1;

    Poller poller;

    const std::vector<ServerConfig> &serversCfg = parser.getServers();

    std::vector<ServerManager*> servers;
    std::map<int, int> listenFdToServerIndex; // listenFd -> index in serversCfg
    std::map<int, Client> clients;

    // start servers
    for (size_t i = 0; i < serversCfg.size(); i++)
    {
        ServerManager *sm = new ServerManager();
        if (!sm->start(serversCfg[i].port))
        {
            Logger::error("Failed to start port " + Logger::toStringInt(serversCfg[i].port));
            return 1;
        }
        int listenFd = sm->getListenFd();
        servers.push_back(sm);
        listenFdToServerIndex[listenFd] = (int)i;
        poller.addFd(listenFd, POLLIN);
    }

    while (true)
    {
        int ready = poller.wait(1000);
        if (ready < 0)
            continue;

        std::vector<struct pollfd> &pfds = poller.getPollFds();

        for (size_t k = 0; k < pfds.size(); k++)
        {
            if (pfds[k].revents == 0)
                continue;

            int fd = pfds[k].fd;

            // if fd is a listen fd → accept
            if (listenFdToServerIndex.find(fd) != listenFdToServerIndex.end() &&
                (pfds[k].revents & POLLIN))
            {
                // find which ServerManager owns this listen fd
                // simplest: linear search
                ServerManager *owner = NULL;
                for (size_t s = 0; s < servers.size(); s++)
                {
                    if (servers[s]->getListenFd() == fd)
                        owner = servers[s];
                }
                if (!owner)
                    continue;

                while (true)
                {
                    int cfd = owner->acceptClient();
                    if (cfd < 0)
                        break;

                    clients[cfd] = Client(cfd, fd);
                    clients[cfd].lastActiveTime = (long long)time(NULL);
                    poller.addFd(cfd, POLLIN);
                }
                continue;
            }

            // client handling (read/write) stays as you already have
        }
    }

    return 0;
}
