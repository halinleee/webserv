#include "Socket.hpp"
#include "main.hpp"

Socket::Socket()
{
    this->socketFd = -1;
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(8080);
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->timeState.timeAct = 0;
    this->timeState.timeOut = 0;
    memset(this->addr.sin_zero, 0, sizeof(this->addr.sin_zero));
}

Socket::Socket(int fd, in_port_t port)
{
    this->socketFd = fd;
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->timeState.timeAct = 0;
    this->timeState.timeOut = 0;
    memset(this->addr.sin_zero, 0, sizeof(this->addr.sin_zero));
}

Socket::Socket(int fd, struct sockaddr_in addr)
{
    this->socketFd = fd;
    this->addr = addr;
    this->timeState.timeAct = 0;
    this->timeState.timeOut = 0;
}

void Socket::setTimeStatus(time_t addTime)
{
    this->timeState.timeAct = std::time(NULL);
    this->timeState.timeOut = this->timeState.timeAct + addTime;
}

const int &Socket::getFd(void) const {return (this->socketFd);}

const struct sockaddr_in &Socket::getAddr(void) const { return (this->addr); }

bool Socket::checkTimeOut(void) { return (this->timeState.timeOut < std::time(NULL)); }

Socket::~Socket() 
{
    if (this->socketFd != -1)
        close(this->socketFd);
}