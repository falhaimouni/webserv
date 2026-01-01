#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <vector>
#include <sstream>
#include <poll.h>

class Server
{
    private:
        int _server_fd;
        int _port;
        std::vector<struct pollfd> _fds;

    public:
        Server(int port);
        ~Server();

        void socketSetup();
        void acceptConnections();
        void run();

        int getServerFd() const;
};

#endif
