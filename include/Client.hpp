#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "Pipe.hpp"
#include "Socket.hpp"
#include "RequestParser.hpp"
#include "RouteResult.hpp"
#include "ServerConfig.hpp"
#include "CgiParser.hpp"
#include "Response.hpp"
#include "type.hpp"

#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>

enum PipeFlag
{
    InFlag = 0,
    OutFlag = 1
};

class Client
{
    private:
        Socket *clientSocket;

        bool runCgi;
        CharDq recDq;
        EnvMap env;
        Pipe cgiPipe;
        pid_t pid;

        std::string cgiRawOutput;

        Response cgiResponse;

        RequestParser parser;
        Request request;
        bool shouldClose;

        FD listenFd;
        CgiParser cgiParser;

        RouteResult routeResult;

        size_t sentOffset;

    public:
        Client();

        Client(Socket *socket, EnvMap env);

        ~Client();

        std::string response;

        RetStatus checkCgiExited(void);

        bool checkAlive(void);

        bool checkRunCgi(const LocationConfig &config, const std::string &resolvedPath, int &errorCode);

        bool getRunCgi();

        void timeSet(time_t addTime);

        RetStatus writeCgiPipe(void);

        RetStatus readCgiPipe(void);

        void clearCgiRawOutput(void);

        void setRunCgi(bool value);
        void setPid(pid_t pid);

        void setListenFd(int fd);

        int getListenFd(void) const;

        Pipe &getCgiPipe();

        void fail(Status status, FailMode mode);

        void CharDqAppend(int length, unsigned char *received);

        CharDq &getCharDq(void);

        Socket &getSocket();

        pid_t getPid(void);

        int getPipeFd(int index);

        Request getRequest();

        void pipeClose(int flag);
        ReqParseResult onReceive();
        bool getShouldClose() const;

        bool hasIncompleteRequest() const;

        void setMaxBodyLength(size_t length);

        void setRouteResult(const RouteResult &result);

        const RouteResult &getRouteResult() const;

        void resetForNextRequest();

        const Response &getCgiResponse() const;

        size_t getSentOffset() const;

        void addSentOffset(size_t length);

        void resetSentOffset();
};


#endif