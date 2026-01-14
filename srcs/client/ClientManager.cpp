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

bool ClientManager::hasRequest(int fd) const
{
    if (clients.count(fd))
    {
        return (clients.at(fd).parser.isComplete());
    }
    return false;
}

const Request& ClientManager::getRequest(int fd) const
{
    static const Request empty; // can't be modified

    if (hasRequest(fd))
    {
        return (clients.at(fd).parser.getRequest());
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
    if (clients.count(fd))
    {
        return (clients.at(fd).parser.hasError());
    }
    return false;
}

int ClientManager::getErrorStatus(int fd)
{
    if (clients.count(fd))
        return clients.at(fd).parser.getErrorStatus();
    return HTTP_INTERNAL_SERVER_ERROR;  // 500 - shouldn't happen
}