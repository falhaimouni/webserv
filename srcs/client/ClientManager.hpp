#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <map>
#include "includes/shared/Request.hpp"
#include "HttpParser.hpp"

struct ClientState {
    HttpParser parser; //each client has its own parser
};

class ClientManager {
public:
    void addClient(int fd); // called when accept(), create new clientStatus with fresh HTTPparser
    void removeClient(int fd); //when client disconnects, error, or connection closed
    void onData(int fd, const std::string& bytes); // called when recv(fd), to find the client's parser and feed it data, (bridge between sockets and parsing)
    bool hasRequest(int fd); // check if the request is complete
    Request getRequest(int fd); //return the parsed request only valid after hasRequest is true
    void reset(int fd); // after response is sent

private:
    std::map<int, ClientState> clients; //owner of all connected clients
};

#endif