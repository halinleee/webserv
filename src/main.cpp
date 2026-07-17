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
   //server먼저하면 config는 server한테 파싱줘야하는데 어케줌? config가 제일 먼저 하는게 맞지 않나?
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