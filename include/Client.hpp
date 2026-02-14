#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

struct Client
{
    int fd;
    int listenFd;
    std::string writeBuffer;
    std::string readBuffer;
    long long lastActiveTime;
    bool wantClose;
    bool requestReady;

    Client()
        : fd(-1), listenFd(-1), lastActiveTime(0), wantClose(false) {}

    Client(int clientFd, int listenfd)
        : fd(clientFd), listenFd(listenfd), lastActiveTime(0), wantClose(false) {}
};

#endif
