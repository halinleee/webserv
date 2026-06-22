#include "main.hpp"
#include "Utils.hpp"
#include "Server.hpp"
#include "Epoll.hpp"

int main(int ac, char **av, char **envp)
{
    Server server(envp);
    Epoll epoll;
    if (epoll.getEpollFd() == -1)
        return (-1);
    if (!server.serverAdd(8080, epoll))
        std::cerr << "서버 시작 실패" << std::endl;
    server.eventProcess(epoll);
    (void) ac;
    (void) av;
}