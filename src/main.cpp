#include "Logger.hpp"
#include "ServerManager.hpp"
#include "Poller.hpp"
#include "Client.hpp"
#include "http/HttpHandler.hpp"

#include <unistd.h>
#include <map>
#include <vector>
#include <poll.h>
#include <sys/socket.h>
#include <cstring>
#include <ctime>

int main()
{
    ServerManager server;
    Poller poller;
    std::map<int, Client> clients;

    if (!server.start(8080))
    {
        Logger::error("Server failed to start");
        return 1;
    }

    poller.addFd(server.getListenFd(), POLLIN);

    while (true)
    {
        int ready = poller.wait(1000);
        if (ready < 0)
            continue;

        std::vector<struct pollfd>& pfds = poller.getPollFds();

        for (size_t i = 0; i < pfds.size(); i++)
        {
            if (pfds[i].revents == 0)
                continue;

            int fd = pfds[i].fd;

            // 1) Listener readable -> accept clients
            if (fd == server.getListenFd() && (pfds[i].revents & POLLIN))
            {
                while (true)
                {
                    int clientFd = server.acceptClient();
                    if (clientFd < 0)
                        break;

                    if (clients.find(clientFd) != clients.end())
                    {
                        close(clientFd);
                        continue;
                    }
                    clients[clientFd] = Client(clientFd);
                    clients[clientFd].lastActiveTime = (long long)time(NULL);
                    poller.addFd(clientFd, POLLIN);

                   // Logger::info("Accepted client fd=" + Logger::toStringInt(clientFd));
                }
                continue;
            }

            // 2) Error/hangup
            if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                Logger::error("Client error/hangup fd=" + Logger::toStringInt(fd));
                close(fd);
                poller.removeFd(fd);
                clients.erase(fd);
                break; // pfds changed
            }

            // 3) Readable client
            if (pfds[i].revents & POLLIN)
            {
                char buf[4096];
                int bytes = recv(fd, buf, sizeof(buf), 0);

                if (bytes <= 0)
                {
                    Logger::info("Client disconnected fd=" + Logger::toStringInt(fd));
                    close(fd);
                    poller.removeFd(fd);
                    clients.erase(fd);
                    break;
                }

                Client &c = clients[fd];
                c.lastActiveTime = (long long)time(NULL);
                c.readBuffer.append(buf, bytes);

                if (c.readBuffer.find("\r\n\r\n") != std::string::npos)
                {
                    Logger::info("HTTP headers complete for fd=" + Logger::toStringInt(fd));

                    // Build response
                    HttpHandler::handle(c);

                    // Clear request buffer (for now one request per connection)
                    c.readBuffer.clear();

                    // Enable sending
                    poller.setEvents(fd, POLLIN | POLLOUT);
                }

            }

            // 4) Writable client -> send response
            if (pfds[i].revents & POLLOUT)
            {
                Client &c = clients[fd];

                if (!c.writeBuffer.empty())
                {
                    int sent = send(fd, c.writeBuffer.c_str(), c.writeBuffer.size(), 0);

                    if (sent <= 0)
                    {
                        Logger::info("Send failed/disconnect fd=" + Logger::toStringInt(fd));
                        close(fd);
                        poller.removeFd(fd);
                        clients.erase(fd);
                        break;
                    }

                    c.writeBuffer.erase(0, sent);
                }

                // If finished sending, close (since handler sets wantClose=true)
                if (c.writeBuffer.empty() && c.wantClose)
                {
                    Logger::info("Closing client fd=" + Logger::toStringInt(fd));
                    close(fd);
                    poller.removeFd(fd);
                    clients.erase(fd);
                    break;
                }

                // Otherwise stop watching POLLOUT
                if (c.writeBuffer.empty())
                    poller.setEvents(fd, POLLIN);
            }
        }
    }

    return 0;
}
