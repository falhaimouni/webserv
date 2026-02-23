#include "../../includes/router/ClientManager.hpp"
#include <cstdio>
#include <cstdlib>

//add new client when a connection is accepted
void ClientManager::addClient(int fd)
{
    if (clients.count(fd))
    {
        clients[fd].parser.reset();
    }
    else
    {
        try
        {
            ClientState client;
            clients.insert(std::make_pair(fd, client));
        }
        catch(const std::bad_alloc& e)
        {
            std::cerr << "Failed to allocate memory for client FD " << fd
                      << ": " << e.what() << "\n";
        }
    }
}

void ClientManager::removeClient(int fd)
{
    if (clients.count(fd))
        clients.erase(fd);
}

//feed incoming bytes to the parser
void ClientManager::onData(int fd, const std::string& bytes)
{
    if (!clients.count(fd))
    {
        return; //ignore unknown FD
    }
    clients[fd].parser.feed(bytes);
}

bool ClientManager::hasRequest(int fd) const
{
    std::map<int, ClientState>::const_iterator it = clients.find(fd);
    if (it != clients.end())
    {
        return (it->second.parser.isComplete());
    }
    return false;
}

const Request& ClientManager::getRequest(int fd) const
{
    static const Request empty; // can't be modified

    std::map<int, ClientState>::const_iterator it = clients.find(fd);
    if (hasRequest(fd))
    {
        // Get the request from parser and add client address
        Request& req = const_cast<Request&>(it->second.parser.getRequest());
        req.clientAddr = it->second.clientAddr;
        return req;
    }
    //empty request if it is not completed or exists
    return (empty);
}

void ClientManager::reset(int fd)
{
    if (clients.count(fd))
    {
        clients[fd].parser.reset();
    }
}

bool ClientManager::hasError(int fd)
{
    std::map<int, ClientState>::iterator it = clients.find(fd);
    if (it != clients.end())
    {
        return (it->second.parser.hasError());
    }
    return false;
}

int ClientManager::getErrorStatus(int fd)
{
    std::map<int, ClientState>::iterator it = clients.find(fd);
    if (it != clients.end())
        return it->second.parser.getErrorStatus();
    return HTTP_INTERNAL_SERVER_ERROR;  // 500 - shouldn't happen
}

void ClientManager::handleSession(int fd)
{
    if (!hasRequest(fd))
        return; //request not complete
    
    //get the parse request, to read the headers
    Request& req = const_cast<Request&>(clients[fd].parser.getRequest());

    //check if the request has a cookie header
    std::string cookieHeader;
    if (req.headers.count("cookie"))
    {
        cookieHeader = req.headers["cookie"];

        //serch for session id = xxx
        size_t pos = cookieHeader.find("session_id=");
        if (pos != std::string::npos)
        {
            pos += 11;
            size_t end = cookieHeader.find(';', pos);
            clients[fd].sessionID = cookieHeader.substr(pos, end - pos);
            return; //found session;
        }
    }

    //if session id not found genetate a new one
    char sessionBuf[16];
    std::sprintf(sessionBuf, "%d", (std::rand() % 1000000) + 100000);
    clients[fd].sessionID = std::string(sessionBuf);
}

std::string ClientManager::getSessionID(int fd) const
{
    std::map<int, ClientState>::const_iterator it = clients.find(fd);
    if (it == clients.end())
        return "";
    return it->second.sessionID;
}

void ClientManager::clientClosed(int fd)
{
    std::map<int, ClientState>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    HttpParser &parser = it->second.parser;

    //if it is complete nothing to do
    if (parser.isComplete())
        return;
    //if content length exists and body is no complete
    long long contentLength = parser.getContentLength();
    if (contentLength > 0 && parser.getBodySize() < (size_t)contentLength)
        parser.forceError(HTTP_BAD_REQUEST);
}

void ClientManager::setClientAddr(int fd, const std::string& addr)
{
    if (clients.count(fd))
        clients[fd].clientAddr = addr;
}