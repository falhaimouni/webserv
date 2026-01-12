#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <errno.h>

class Server
{
private:
    int _server_fd;
    int _port;
    std::vector<pollfd> _fds;

public:
    Server(int port);
    ~Server();

    void socketSetup();
    void run();
};

#endif
