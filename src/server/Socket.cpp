#include "Socket.hpp"

Socket::Socket()
{
    this->socketFd = -1;
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(8080);
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->timeAct = 0;
    this->timeOut = 0;
    memset(this->addr.sin_zero, 0, sizeof(this->addr.sin_zero));
}

Socket::Socket(int fd, in_port_t port)
{
    this->socketFd = fd;
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
    memset(this->addr.sin_zero, 0, sizeof(this->addr.sin_zero));
}

Socket::Socket(int fd, struct sockaddr_in addr)
{
    this->socketFd = fd;
    this->addr = addr;
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
}

void Socket::actTimeSet(void)
{
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
}

const int &Socket::getFd(void) const {return (this->socketFd);}

const struct sockaddr_in &Socket::getAddr(void) const { return (this->addr); }

const time_t &Socket::getTimeOut(void) const { return (this->timeOut); }

Socket::~Socket() 
{
    if (this->socketFd != -1)
        close(this->socketFd);
}