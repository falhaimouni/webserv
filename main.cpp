#include <iostream>
#include <map>
#include <vector>
#include <ctime>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>

#include "handlers/ResponseBuilder.hpp"
#include "handlers/ErrorHandler.hpp"
#include "handlers/RequestRouter.hpp"

#include "router/ClientManager.hpp"
#include "router/Router.hpp"

#include "server/ConfigParser.hpp"
#include "server/ServerManager.hpp"
#include "server/Poller.hpp"
#include "server/Logger.hpp"
#include "server/ServerConfig.hpp"

static volatile sig_atomic_t stopFlag = 0;
void signalHandler(int signum)
{
    if (signum == SIGINT)
        stopFlag = 1;
}

static std::string reasonPhrase(int status)
{
    switch (status)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "Error";
    }
}

static LocationConfig buildDefaultLocation(const ServerConfig &srv)
{
    LocationConfig loc;
    loc.root = srv.root;
    loc.index = srv.index;
    loc.error_pages = srv.error_pages;
    loc.max_body_size = srv.client_max_body_size;
    return loc;
}

static void normalizeServerConfigs(std::vector<ServerConfig> &servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].locations.empty())
        {
            servers[i].locations.push_back(buildDefaultLocation(servers[i]));
        }
        for (size_t j = 0; j < servers[i].locations.size(); ++j)
        {
            if (servers[i].locations[j].root.empty())
                servers[i].locations[j].root = servers[i].root;
            if (servers[i].locations[j].index.empty())
                servers[i].locations[j].index = servers[i].index;
            if (servers[i].locations[j].error_pages.empty())
                servers[i].locations[j].error_pages = servers[i].error_pages;
            if (servers[i].locations[j].max_body_size == 0)
                servers[i].locations[j].max_body_size = servers[i].client_max_body_size;
        }

        // Auto-enable directory listing for /uploads and ensure upload_store root
        bool hasUploadPath = false;
        bool hasStaticLocation = false;
        bool hasRedirectLocation = false;

        for (size_t j = 0; j < servers[i].locations.size(); ++j)
        {
            if (servers[i].locations[j].path == "/uploads")
            {
                servers[i].locations[j].autoindex = true;
                if (!servers[i].locations[j].upload_store.empty())
                    servers[i].locations[j].root = servers[i].locations[j].upload_store;
            }
            if (servers[i].locations[j].path == "/upload")
            {
                hasUploadPath = true;
                if (servers[i].locations[j].methods.empty())
                    servers[i].locations[j].methods.insert("POST");
                servers[i].locations[j].upload_enable = true;
            }
            if (servers[i].locations[j].path == "/static")
                hasStaticLocation = true;
            if (servers[i].locations[j].path == "/redirect")
                hasRedirectLocation = true;
        }

        // Add missing default test routes if not in config
        if (!hasUploadPath)
        {
            LocationConfig up;
            up.path = "/upload";
            up.root = servers[i].root;
            up.index = servers[i].index;
            up.error_pages = servers[i].error_pages;
            up.max_body_size = servers[i].client_max_body_size;
            up.upload_enable = true;
            up.upload_store = servers[i].root + "/uploads";
            up.methods.insert("POST");
            servers[i].locations.push_back(up);
        }

        if (!hasStaticLocation)
        {
            LocationConfig st;
            st.path = "/static";
            st.root = servers[i].root + "/testdir";
            st.index = servers[i].index;
            st.error_pages = servers[i].error_pages;
            st.max_body_size = servers[i].client_max_body_size;
            st.methods.insert("GET");
            servers[i].locations.push_back(st);
        }

        if (!hasRedirectLocation)
        {
            LocationConfig rd;
            rd.path = "/redirect";
            rd.root = servers[i].root;
            rd.index = servers[i].index;
            rd.error_pages = servers[i].error_pages;
            rd.max_body_size = servers[i].client_max_body_size;
            rd.redirect_to = "/";
            rd.redirect_code = 301;
            rd.methods.insert("GET");
            servers[i].locations.push_back(rd);
        }
    }
}

static void closeClient(int fd,
                        Poller &poller,
                        ClientManager &clientManager,
                        std::map<int, int> &clientFdToServerIndex,
                        std::map<int, std::string> &pendingResponses)
{
    poller.removeFd(fd);
    clientManager.removeClient(fd);
    clientFdToServerIndex.erase(fd);
    pendingResponses.erase(fd);
    close(fd);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signalHandler);

    if (argc != 2)
    {
        Logger::error("Usage: ./webserv <config_file>");
        return 1;
    }

    ConfigParser parser;
    if (!parser.parse(argv[1]))
        return 1;

    Poller poller;

    std::vector<ServerConfig> serversCfg = parser.getServers();
    normalizeServerConfigs(serversCfg);

    std::vector<ServerManager*> servers;
    std::map<int, int> listenFdToServerIndex; // listenFd -> index in serversCfg

    ClientManager clientManager;
    Router router;
    std::map<int, int> clientFdToServerIndex;
    std::map<int, std::string> pendingResponses;

    // start servers
   
    for (size_t i = 0; i < serversCfg.size(); i++)
    {
        ServerManager *sm = new ServerManager();
        if (!sm->start(serversCfg[i].host, serversCfg[i].port))
        {
            Logger::error("Failed to start port " + Logger::toStringInt(serversCfg[i].port));
            delete sm;
            for (size_t j = 0; j < servers.size(); ++j)
                delete servers[j];
            return 1;
        }
        int listenFd = sm->getListenFd();
        servers.push_back(sm);
        listenFdToServerIndex[listenFd] = (int)i;
        poller.addFd(listenFd, POLLIN);
    }
    
    while (!stopFlag)
    {
        int ready = poller.wait(1000);
        if (ready < 0)
            continue;

        std::vector<int> toClose;
        std::vector<struct pollfd> &pfds = poller.getPollFds();

        for (size_t k = 0; k < pfds.size(); k++)
        {
            if (pfds[k].revents == 0)
                continue;

            int fd = pfds[k].fd;

            // if fd is a listen fd → accept
            if (listenFdToServerIndex.find(fd) != listenFdToServerIndex.end() &&
                (pfds[k].revents & POLLIN))
            {
                int serverIndex = listenFdToServerIndex[fd];
                ServerManager *owner = servers[serverIndex];

                while (true)
                {
                    int cfd = owner->acceptClient();
                    if (cfd < 0)
                        break;

                    clientManager.addClient(cfd);
                    clientFdToServerIndex[cfd] = serverIndex;
                    poller.addFd(cfd, POLLIN);
                }
                continue;
            }

            if (pfds[k].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                toClose.push_back(fd);
                continue;
            }

            if (pfds[k].revents & POLLIN)
            {
                char buffer[65536];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                if (bytes == 0)
                {
                    //client close the connection and the body was not fully received
                    clientManager.clientClosed(fd);
                    toClose.push_back(fd);
                    continue;
                }
                if (bytes < 0)
                {
                    toClose.push_back(fd);
                    continue;
                }

                clientManager.onData(fd, std::string(buffer, bytes));

                if (clientManager.hasError(fd) || clientManager.hasRequest(fd))
                {
                    int serverIndex = clientFdToServerIndex[fd];
                    const ServerConfig &srv = serversCfg[serverIndex];
                    LocationConfig fallback = buildDefaultLocation(srv);

                    Response res;
                    std::string httpVersion = "HTTP/1.0";

                    if (clientManager.hasError(fd))
                    {
                        int status = clientManager.getErrorStatus(fd);
                        res = ErrorHandler::handleError(status, reasonPhrase(status), fallback);
                    }
                    else
                    {
                        clientManager.handleSession(fd);
                        const Request &req = clientManager.getRequest(fd);

                        int routeStatus = 0;
                        const LocationConfig *loc = router.route(req, srv.locations, routeStatus);
                        if (!loc)
                            res = ErrorHandler::handleError(routeStatus, reasonPhrase(routeStatus), fallback);
                        else
                            res = routeRequest(req, *loc);
                    }

                    std::string sessionId = clientManager.getSessionID(fd);
                    pendingResponses[fd] = ResponseBuilder::formatResponse(res, httpVersion, sessionId);
                    poller.setEvents(fd, POLLOUT);
                }
            }

            if (pfds[k].revents & POLLOUT)
            {
                std::map<int, std::string>::iterator it = pendingResponses.find(fd);
                if (it == pendingResponses.end())
                {
                    toClose.push_back(fd);
                    continue;
                }

                std::string &out = it->second;
                if (out.empty())
                {
                    toClose.push_back(fd);
                    continue;
                }

                ssize_t sent = send(fd, out.c_str(), out.size(), MSG_NOSIGNAL);
                if (sent < 0)
                {
                    toClose.push_back(fd);
                    continue;
                }
                if (sent == 0)
                {
                    toClose.push_back(fd);
                    continue;
                }

                out.erase(0, sent);
                if (out.empty())
                    toClose.push_back(fd);
            }
        }
        // for (size_t i = 0; i < toClose.size(); ++i)
        //     closeClient(toClose[i], poller, clientManager, clientFdToServerIndex, pendingResponses);
     }
    
        // Collect remaining fds from poller
        std::vector<int> remainingFds;
        std::vector<struct pollfd> &pfds_final = poller.getPollFds();
        for (size_t i = 0; i < pfds_final.size(); ++i)
            remainingFds.push_back(pfds_final[i].fd);

        // Close non-listen fds properly
        for (size_t i = 0; i < remainingFds.size(); ++i)
        {
            int fd = remainingFds[i];
            if (listenFdToServerIndex.find(fd) == listenFdToServerIndex.end())
            {
                if (clientFdToServerIndex.find(fd) != clientFdToServerIndex.end() ||
                    pendingResponses.find(fd) != pendingResponses.end())
                {
                    closeClient(fd, poller, clientManager, clientFdToServerIndex, pendingResponses);
                }
                else
                {
                    // fallback: remove from poller and close
                    poller.removeFd(fd);
                    close(fd);
                }
            }
        }

        // Close listen sockets and delete ServerManager instances
        for (size_t i = 0; i < servers.size(); ++i)
        {
            if (servers[i])
            {
                int lfd = servers[i]->getListenFd();
                poller.removeFd(lfd);
                close(lfd);
                delete servers[i];
            }
        }
        servers.clear();
        listenFdToServerIndex.clear();

    return 0;
}

