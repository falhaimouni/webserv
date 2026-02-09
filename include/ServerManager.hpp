#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <iostream>

class ServerManager {
    private:
        int _listenFd;
        bool setNonBlocking(int fd);
public:
    ServerManager();
    ~ServerManager();

    bool start(int port);
    int  getListenFd() const;

    // Day 3 helper: accept one client (returns client fd or -1)
    int  acceptClient();
};

#endif