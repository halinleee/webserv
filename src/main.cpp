#include "Server.hpp"
#include "Epoll.hpp"
#include "Config.hpp"

#include <csignal>
#include <iostream>

Server *serverPointer;

static void sigIntHandler(int signum)
{
    (void)signum;
    serverPointer->serverClose();
    std::cout << "Server Close" << std::endl;
}

int main(int ac, char **av)
{
    Config config;
    if (!config.parseConfig(ac, av))
    {
        std::cerr << config.getStatusMessage() << std::endl;
        return 1;
    }
    Server server;
    Epoll epoll;
    serverPointer = &(server);

    signal(SIGINT, sigIntHandler);
    signal(SIGPIPE, SIG_IGN);
    if (epoll.getEpollFd() == -1)
        return -1;
    if (!server.serverAdd(config.getConfig(), epoll))
    {
        std::cerr << "서버 시작 실패" << std::endl;
        return 1;
    }
    server.eventProcess(epoll);
}