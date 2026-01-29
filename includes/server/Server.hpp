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
#include "../router/ClientManager.hpp"
#include "../router/Router.hpp"
#include "../shared/LocationConfig.hpp"

class Server
{
private:
    int _server_fd;
    int _port;
    std::vector<pollfd> _fds;
    ClientManager _clientManager;
    Router _router;
    std::vector<LocationConfig> _locations;

public:
    Server(int port);
    ~Server();

    void socketSetup();
    void run();
    void initializeLocations();
};

#endif
