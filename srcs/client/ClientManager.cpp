#include "../../includes/router/ClientManager.hpp"

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

bool ClientManager::hasRequest(int fd)
{
    if (clients.count(fd))
    {
        return (clients.at(fd).parser.isComplete());
    }
    return false;
}

Request ClientManager::getRequest(int fd)
{
    if (hasRequest(fd))
    {
        return (clients.at(fd).parser.getRequest());
    }
    //empty request if it is not completed or exists
    return Request();
}

void ClientManager::reset(int fd)
{
    if (clients.count(fd))
    {
        clients[fd].parser.reset();
    }
}
