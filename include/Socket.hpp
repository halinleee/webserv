#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <netinet/in.h>
#include <ctime>

struct timeState
{
    time_t timeAct;
    time_t timeOut;
};

class Socket
{
    private:
        int socketFd;
        struct sockaddr_in addr;

        struct  timeState timeState;

    public:
        Socket();

        Socket(int fd, uint32_t ip, in_port_t port);

        Socket(int fd, struct sockaddr_in addr);

        ~Socket();

        const int &getFd(void) const;

        const struct sockaddr_in &getAddr(void) const;

        bool checkTimeOut(void);

        void setTimeStatus(time_t addTime);
};



#endif