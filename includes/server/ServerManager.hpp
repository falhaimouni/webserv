#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <iostream>
#include <string>

class ServerManager {
    private:
        int _listenFd;
        std::string _lastClientAddr; // Store last accepted client's IP
        bool setNonBlocking(int fd);
    public:
        ServerManager();
        ~ServerManager();

        bool start(const std::string &host, int port);
        int  getListenFd() const;

        int  acceptClient(); // it returns client fd or -1
        std::string getLastClientAddr() const; // Get the IP address of last accepted client
};

#endif