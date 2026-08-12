#ifndef SERVER_HPP
# define SERVER_HPP

#include "type.hpp"
#include "Epoll.hpp"
#include "Utils.hpp"
#include "Socket.hpp"
#include "Client.hpp"
#include "Cgi.hpp"
#include "ServerConfig.hpp"
#include "Logger.hpp"

#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <algorithm>
#include <arpa/inet.h>
#include <map>
#include <vector>

class Server
{
    private:
        bool serverActive;


        std::map<int, ServerConfig> configs;
        std::vector<Socket *> serverSockets;
        ClientVec client;

        FdVec inClientVec;
        IntMap pipeToClientMap;
        EnvMap env;
        struct timeValue timeOutValue;

    public:
        Server();

        ~Server();

        RetStatus serverAdd (in_port_t port, Epoll &epoll, ServerConfig config);

        RetStatus serverAdd (const std::map<in_port_t, ServerConfig> &configs, Epoll &epoll);

        RetStatus serverSend(Epoll &epoll, Client *client);


        RetStatus eventProcess(Epoll &epoll);

        RetStatus serverSetting(Socket *serverSocket);

        Socket *findServerSocket(FD fd);

        RetStatus clientAccept(Epoll &epoll, Socket *socket);

        RetStatus clientLoop(Epoll &epoll, FD currentFd, u_int32_t currentEvent);

        RetStatus cgiEventLoop(Epoll &epoll, Client *pipeClient, FD currentFd, u_int32_t currentEvent);

        RetStatus clientRequest(Epoll &epoll, Client *client);

        RetStatus clientResponse(Epoll &epoll, Client *client);

        bool clientExist(int fd);

        RetStatus cgiRun(Epoll &epoll, Client *client);

        RetStatus cgiPipeRead(Epoll &epoll, Client *client);

        RetStatus cgiPipeWrite(Epoll &epoll, Client *client);

        void checkTimeOutClient(Epoll &epoll, int &index);

        void deleteClient(int deleteFd);

        void drainSocket(FD fd);

        std::string getResponse(void);

        RetStatus errorHandling(Client *client, Epoll &eopll, int statusCode, FailMode mode);

        RetStatus epollGuard(Epoll &epoll, int op, FD fd, u_int32_t event, Client *client);

        void serverClose();

        void reapCgiChild(pid_t pid);

        void cgiRollback(Client *client, pid_t pid);

        RetStatus cgiTimeoutAbort(Epoll &epoll, Client *client);

        RetStatus readTimeoutAbort(Epoll &epoll, Client *client);

        bool checkMemoryLimit(Client *client);

        std::string buildAccessLog(Client *client, Status statusCode, size_t bodySize) const;
};

#endif