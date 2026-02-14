#include "../../includes/server/Server.hpp"
#include "../../includes/handlers/RequestRouter.hpp"
#include "../../includes/handlers/ResponseBuilder.hpp"
#include <cstring>

Server::Server(int port) : _server_fd(-1), _port(port)
{
    initializeLocations();
}

Server::~Server()
{
    if (_server_fd != -1)
        close(_server_fd);
}

void Server::initializeLocations()
{
    // Default location config
    LocationConfig defaultLoc;
    defaultLoc.path = "/";
    defaultLoc.root = "./www";
    defaultLoc.index = "index.html";
    defaultLoc.autoindex = true;
    defaultLoc.upload_store = "./www/uploads";
    defaultLoc.is_cgi = false;
    defaultLoc.methods.insert("GET");
    defaultLoc.methods.insert("POST");
    defaultLoc.methods.insert("DELETE");
    defaultLoc.cgi_ext.insert(".py");
    defaultLoc.cgi_ext.insert(".php");
    defaultLoc.cgi_ext.insert(".pl");
    defaultLoc.cgi_ext.insert(".sh");
    
    _locations.push_back(defaultLoc);
    
    // Upload location config
    LocationConfig uploadLoc;
    uploadLoc.path = "/upload";
    uploadLoc.root = "./www";
    uploadLoc.index = "index.html";
    uploadLoc.autoindex = true;
    uploadLoc.upload_store = "./www/uploads";
    uploadLoc.is_cgi = false;
    uploadLoc.methods.insert("POST");
    uploadLoc.methods.insert("GET");
    
    _locations.push_back(uploadLoc);
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

                // Add client to ClientManager
                _clientManager.addClient(client_fd);

                std::cout << "Accepted client fd: " << client_fd << std::endl;
            }
            // 🔵 Client request
            else
            {
                char buffer[4096];
                ssize_t bytes = recv(_fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes <= 0)
                {
                    // Remove from ClientManager
                    _clientManager.removeClient(_fds[i].fd);
                    close(_fds[i].fd);
                    _fds.erase(_fds.begin() + i);
                    --i;
                    continue;
                }

                buffer[bytes] = '\0';

                // 🔹 Feed data to ClientManager's parser
                _clientManager.onData(_fds[i].fd, std::string(buffer, bytes));

                // Check if we have a complete request
                if (_clientManager.hasError(_fds[i].fd))
                {
                    // Handle parsing error
                    int errorStatus = _clientManager.getErrorStatus(_fds[i].fd);
                    Response errorRes = ErrorHandler::handleError(errorStatus, "");
                    std::string raw = ResponseBuilder::formatResponse(
                        errorRes,
                        "HTTP/1.0",
                        _clientManager.getSessionID(_fds[i].fd)
                    );
                    send(_fds[i].fd, raw.c_str(), raw.size(), 0);
                    
                    _clientManager.removeClient(_fds[i].fd);
                    close(_fds[i].fd);
                    _fds.erase(_fds.begin() + i);
                    --i;
                    continue;
                }

                if (_clientManager.hasRequest(_fds[i].fd))
                {
                    // 🔹 Get parsed request
                    const Request& req = _clientManager.getRequest(_fds[i].fd);

                    // 🔹 Handle session (parse or create sessionID)
                    _clientManager.handleSession(_fds[i].fd);

                    // 🔹 Route request to find matching location
                    int routeStatus = HTTP_OK;
                    const LocationConfig* matchedLoc = _router.route(req, _locations, routeStatus);

                    Response res;
                    
                    if (matchedLoc == NULL)
                    {
                        // No matching location or method not allowed
                        res = ErrorHandler::handleError(routeStatus, "");
                    }
                    else
                    {
                        // 🔹 Route to appropriate handler
                        res = routeRequest(req, *matchedLoc);
                    }

                    // 🔹 Build and send response
                    std::string raw = ResponseBuilder::formatResponse(
                        res,
                        "HTTP/1.0",
                        _clientManager.getSessionID(_fds[i].fd)
                    );
                    send(_fds[i].fd, raw.c_str(), raw.size(), 0);

                    // 🔹 Reset parser for potential keep-alive
                    _clientManager.reset(_fds[i].fd);

                    // Close connection (for now, no keep-alive)
                    _clientManager.removeClient(_fds[i].fd);
                    close(_fds[i].fd);
                    _fds.erase(_fds.begin() + i);
                    --i;
                }
            }
        }
    }
}
