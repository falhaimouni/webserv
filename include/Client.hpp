#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

struct Client
{
    int fd;
    std::string writeBuffer;
    std::string readBuffer;
    long long lastActiveTime;
    bool wantClose;
    bool requestReady;

    Client()
        : fd(-1), writeBuffer(""), readBuffer(""), lastActiveTime(0), wantClose(false), requestReady(false) {}

    Client(int clientFd)
        : fd(clientFd), writeBuffer(""), readBuffer(""), lastActiveTime(0), wantClose(false), requestReady(false) {}
};

#endif
