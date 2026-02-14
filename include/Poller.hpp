#ifndef POLLER_HPP
#define POLLER_HPP

#include <vector>
#include <poll.h>

class Poller {
    private:
        std::vector<struct pollfd> _pfds;

    public:
        Poller();
        ~Poller();

        void addFd(int fd, short events);
        void removeFd(int fd);
        void setEvents(int fd, short events);

        int  wait(int timeoutMs);

        std::vector<struct pollfd>& getPollFds();
};

#endif
