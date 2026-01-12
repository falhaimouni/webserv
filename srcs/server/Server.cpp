#include "../../includes/server/Server.hpp"
#include "../../includes/handlers/RequestParser.hpp"
#include "../../includes/handlers/RequestRouter.hpp"
#include "../../includes/handlers/ResponseBuilder.hpp"

Server::Server(int port) : _server_fd(-1), _port(port) {}

Server::~Server()
{
    if (_server_fd != -1)
        close(_server_fd);
}

void Server::socketSetup()
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw std::runtime_error("socket failed");

    int opt = 1;
    setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");

    if (listen(_server_fd, SOMAXCONN) < 0)
        throw std::runtime_error("listen failed");

    fcntl(_server_fd, F_SETFL, O_NONBLOCK);

    pollfd pfd;
    pfd.fd = _server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    std::cout << "Server running on port " << _port << std::endl;
}

void Server::run()
{
    while (true)
    {
        if (poll(&_fds[0], _fds.size(), -1) < 0)
            continue;

        for (size_t i = 0; i < _fds.size(); ++i)
        {
            if (!(_fds[i].revents & POLLIN))
                continue;

            // 🟢 New connection
            if (_fds[i].fd == _server_fd)
            {
                sockaddr_in client;
                socklen_t len = sizeof(client);
                int client_fd = accept(_server_fd, (sockaddr *)&client, &len);
                if (client_fd < 0)
                    continue;

                fcntl(client_fd, F_SETFL, O_NONBLOCK);

                pollfd cfd;
                cfd.fd = client_fd;
                cfd.events = POLLIN;
                cfd.revents = 0;
                _fds.push_back(cfd);

                std::cout << "Accepted client fd: " << client_fd << std::endl;
            }
            // 🔵 Client request
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

                // 🔹 Parse
                Request req = RequestParser::parse(buffer);

                // 🔹 Route
                Response res = routeRequest(req);

                // 🔹 Build response
                std::string raw = ResponseBuilder::formatResponse(res);

                // 🔹 Send
                send(_fds[i].fd, raw.c_str(), raw.size(), 0);

                close(_fds[i].fd);
                _fds.erase(_fds.begin() + i);
                --i;
            }
        }
    }
}
