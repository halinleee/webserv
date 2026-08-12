#ifndef EPOLL_HPP
# define EPOLL_HPP

#include <sys/epoll.h>
#include <sys/types.h>

class Epoll
{
    private:
        int epollFd;
        epoll_event event;
        epoll_event events[50];

    public:
        Epoll();

        ~Epoll();

        int getEpollFd();


        epoll_event &operator[] (unsigned int i);

        bool epollControl(int option, int appendFd, u_int64_t events);

        int epWait(void);
};

#endif