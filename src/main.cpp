#include "main.hpp"
#include "Utils.hpp"
#include "Server.hpp"
#include "Epoll.hpp"
#include "Config.hpp"

Server *serverPointer;

static void sigIntHandler(int signum)
{
    (void)signum;
    serverPointer->serverClose();
    std::cout << "Server Close" << std::endl;
}

int main(int ac, char **av, char **envp)
{
    timeValue timeValue;
    timeValue.connetionTimeOut = 60;
    timeValue.readTimeout = 60;
    timeValue.writeTimeout = 60;
    timeValue.keepAliveTimeout = 60;
    timeValue.cgiTimeout = 5;
    Server server(envp, timeValue);
    Epoll epoll;
    serverPointer = &(server);

    Config config;
    if (!config.parseConfig(ac, av))
    {
        std::cerr << config.getStatusMessage() << std::endl;
        return 1;
    }
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