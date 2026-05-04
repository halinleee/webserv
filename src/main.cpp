#include "main.hpp"
#include "Utils.hpp"
#include "Server.hpp"
#include "Epoll.hpp"

Server *serverPointer;

static void sigIntHandler(int signum)
{
    (void)signum;
    serverPointer->serverClose();
    std::cout << "Server Close" << std::endl;
}

int main(int ac, char **av, char **envp)
{
    Server server(envp);
    Epoll epoll;
    serverPointer = &(server);
    signal(SIGINT, sigIntHandler);
    if (epoll.getEpollFd() == -1)
        return (-1);
    if (!server.serverAdd(8080, epoll))
        std::cerr << "서버 시작 실패" << std::endl;
    server.eventProcess(epoll);
    close(epoll.getEpollFd());
    (void) ac;
    (void) av;
}