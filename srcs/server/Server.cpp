#include "../../includes/server/Server.hpp"
#include "../../includes/handlers/RequestParser.hpp"
#include "../../includes/handlers/ResponseBuilder.hpp"
#include "../../includes/handlers/RequestHandler.hpp"
#include "../../includes/handlers/RequestRouter.hpp"


Response routeRequest(const Request &req);

Server::Server(int port) : _server_fd(-1), _port(port) {}

Server::~Server()
{
    if (_server_fd != -1)
        close(_server_fd);
}

int Server::getServerFd() const
{
    return _server_fd;
}

void Server::socketSetup()
{
    // Create socket
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw std::runtime_error("socket() failed");

    // Allow reuse of address
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(_server_fd);
        throw std::runtime_error("setsockopt() failed");
    }

    sockaddr_in server_addr = sockaddr_in();

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(_port); // from config file

    if (bind(_server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(_server_fd);
        throw std::runtime_error("bind() failed");
    }

    if (listen(_server_fd, SOMAXCONN) < 0)
    {
        close(_server_fd);
        throw std::runtime_error("listen() failed");
    }

    // Non-blocking mode (REQUIRED for select/poll/epoll)
    if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(_server_fd);
        throw std::runtime_error("fcntl() failed");
    }
    pollfd pfd;
    pfd.fd = _server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);
}

void Server::acceptConnections()
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(_server_fd, (sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            std::cerr << "accept() failed: " << strerror(errno) << std::endl;
        return;
    }

    // Set client socket non-blocking
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "fcntl() client failed\n";
        close(client_fd);
        return;
    }
    char buffer[4096];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        Request req = RequestParser::parse(buffer);
        Response res = routeRequest(req);
        std::string rawResponse = ResponseBuilder::formatResponse(res);
        send(client_fd, rawResponse.c_str(), rawResponse.size(), 0);
    }

    close(client_fd);
    std::cout << "Accepted client fd: " << client_fd << std::endl;
    
}

void Server::run()
{
    while (true)
    {
        if (poll(&_fds[0], _fds.size(), -1) < 0)
        {
            std::cerr << "poll() failed\n";
            break;
        }

        for (size_t i = 0; i < _fds.size(); ++i)
        {
            if (_fds[i].revents & POLLIN)
            {
                // 1️⃣ New connection
                if (_fds[i].fd == _server_fd)
                {
                    sockaddr_in client_addr;
                    socklen_t len = sizeof(client_addr);

                    int client_fd = accept(_server_fd, (sockaddr *)&client_addr, &len);
                    if (client_fd >= 0)
                    {
                        // Set non-blocking
                        fcntl(client_fd, F_SETFL, O_NONBLOCK);

                        pollfd client_pfd;
                        client_pfd.fd = client_fd;
                        client_pfd.events = POLLIN;
                        client_pfd.revents = 0;
                        _fds.push_back(client_pfd);
                        std::cout << "Accepted client fd: " << client_fd << std::endl;
                    }
                }
                // 2️⃣ Client sent data
                else
                {
                    char buffer[4096];
                    ssize_t bytes = recv(_fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytes <= 0)
                    {
                        close(_fds[i].fd);
                        _fds.erase(_fds.begin() + i);
                        --i;
                        continue;
                    }

                    buffer[bytes] = '\0';

                    // 1️⃣ Parse request
                    Request req = RequestParser::parse(buffer);

                    // 2️⃣ Route request
                    Response res = routeRequest(req);

                    // 3️⃣ Send response
                    std::string responseStr = ResponseBuilder::formatResponse(res);
                    send(_fds[i].fd, responseStr.c_str(), responseStr.size(), 0);

                    // 4️⃣ Close client connection
                    close(_fds[i].fd);
                    _fds.erase(_fds.begin() + i);
                    --i;

                }
            }
        }
    }
}
