#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <map>
#include <new> // std::bad_alloc
#include <iostream>
#include <cstdlib> // rand()
#include <ctime> // time()
#include "../shared/Request.hpp"
#include "HttpParser.hpp"

struct ClientState {
    HttpParser parser; //each client has its own parser
    std::string sessionID; //store session id for this client
    std::string clientAddr; //client IP address
};

class ClientManager {
public:
    void addClient(int fd); // called when accept(), create new clientStatus with fresh HTTPparser
    void removeClient(int fd); //when client disconnects, error, or connection closed
    void onData(int fd, const std::string& bytes); // called whenever poll() tells you there is data for this client, (bridge between sockets and parsing)
    bool hasRequest(int fd) const; // check if the request is complete
    const Request& getRequest(int fd) const; //return the parsed request only valid after hasRequest is true
    bool hasError(int fd); // check if there was a parsing error
    void reset(int fd); // after response is sent
    int getErrorStatus(int fd);
    void handleSession(int fd); //call after request is fully parsed
    std::string getSessionID(int fd) const;
    void clientClosed(int fd);
    void setClientAddr(int fd, const std::string& addr); // store client IP address

private:
    std::map<int, ClientState> clients; //owner of all connected clients
};

#endif