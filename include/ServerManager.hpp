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

        int  acceptClient(); // it returns client fd or -1
};

#endif