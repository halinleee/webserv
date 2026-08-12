#include "Epoll.hpp"
#include "type.hpp"

#include <unistd.h>
#include <fcntl.h>

Epoll::Epoll()
{
    this->epollFd = epoll_create(8192);
    fcntl(epollFd, F_SETFD, FD_CLOEXEC);
}

bool Epoll::epollControl(int option, int appendFd, u_int64_t events)
{
    event.data.fd = appendFd;
    event.events = events;
    if (epoll_ctl(this->epollFd, option, appendFd, &event) < 0)
        return RET_ERROR;
    return RET_OK;
}

Epoll::~Epoll()
{
    close(epollFd);
}

int Epoll::getEpollFd()
{
    return epollFd;
}

epoll_event &Epoll::operator[] (unsigned int i)
{
    return events[i];
}

int Epoll::epWait(void)
{
    int eventCount = epoll_wait(this->epollFd, this->events, 50, 20);
    if (eventCount < 0)
        return -1;
    return eventCount;
}

