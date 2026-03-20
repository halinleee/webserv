#include "main.hpp"
#include "Utils.hpp"
#include "Server.hpp"
#include "Epoll.hpp"

int main(int ac, char **av, char **envp)
{
    Server server(envp);
    Epoll epoll;

    server.serverAdd(8080, epoll);
    server.eventProcess(epoll);
    (void) ac;
    (void) av;
}