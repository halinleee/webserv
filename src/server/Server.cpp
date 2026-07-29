#include "Server.hpp"
#include "Pipe.hpp"
#include "main.hpp"
#include "Response.hpp"
#include "Router.hpp"
#include "Handler.hpp"

Server::Server(char **envp) : serverActive(true), client(8192, NULL), env(envpParsing(envp)), timeOutValue() {}

Server::~Server()
{
    for (ClientVec::iterator it = this->client.begin(); it != this->client.end(); ++it)
        delete *it;
    for (std::vector<Socket *>::iterator it = this->serverSockets.begin(); it != this->serverSockets.end(); ++it)
        delete *it;
}

RetStatus Server::serverAdd(in_port_t port, Epoll &epoll, ServerConfig config)
{
    int socketFd;
    Socket *tmpSocket;
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {return RET_ERROR;}
    tmpSocket = new Socket(socketFd, port);
    this->timeOutValue = config.getTimeConfig();
    if (!serverSetting(tmpSocket)) {return RET_ERROR;}
    if (!epoll.epollControl(EPOLL_CTL_ADD, tmpSocket->getFd(), EPOLLIN))
    {
        delete tmpSocket;
        return RET_ERROR;
    }
    this->configs[tmpSocket->getFd()] = config;
    this->serverSockets.push_back(tmpSocket);
    return RET_OK;
}

RetStatus Server::serverAdd(const std::map<in_port_t, ServerConfig> &configs, Epoll &epoll)
{
    for (std::map<in_port_t, ServerConfig>::const_iterator it = configs.begin(); it != configs.end(); ++it)
    {
        if (!serverAdd(it->first, epoll, it->second))
            return RET_ERROR;
    }
    return RET_OK;
}

RetStatus Server::eventProcess(Epoll &epoll)
{
    FD currentFd;
    u_int64_t currentEvent;
    int eventCount = 0;
    int index = 0;
    while(serverActive)
    {
        if ((eventCount = epoll.epWait()) < 0)
        {
            if (!serverActive)
                return RET_OK;
            return RET_ERROR;
        }
        for (int i = 0; i < eventCount; i++)
        {
            currentFd = epoll[i].data.fd;
            currentEvent = epoll[i].events;
            Socket *acceptSocket = findServerSocket(currentFd);
            if (acceptSocket && (currentEvent & EPOLLIN))
            {
                if (!clientAccept(epoll, acceptSocket))
                    continue;
            }
            else if (!clientLoop(epoll, currentFd, currentEvent))
                continue;
        }
        if (this->inClientVec.size() > 0)
            checkTimeOutClient(epoll, index);
    }
    return RET_OK;
}

Socket *Server::findServerSocket(FD fd)
{
    for (std::vector<Socket *>::iterator it = this->serverSockets.begin(); it != this->serverSockets.end(); ++it)
    {
        if ((*it)->getFd() == fd)
            return *it;
    }
    return NULL;
}

RetStatus Server::clientLoop(Epoll &epoll, FD currentFd, u_int32_t currentEvent)
{
    if (this->pipeToClientMap.count(currentFd) > 0 && clientExist(this->pipeToClientMap[currentFd]))
        return cgiEventLoop(epoll, this->client[this->pipeToClientMap[currentFd]], currentFd, currentEvent);
    if (clientExist(currentFd))
    {
        if (currentEvent & EPOLLERR || currentEvent & EPOLLHUP)
        {
            epoll.epollControl(EPOLL_CTL_DEL, currentFd, 0);
            deleteClient(currentFd);
            return RET_ERROR;
        }
        if (currentEvent & EPOLLIN)
        {
            std::cout << currentFd << "EPOLL_IN" << std::endl;
            if (!clientRequest(epoll, this->client[currentFd]))
                std::cerr << "clientRequest Error" << std::endl;
        }
        if (currentEvent & EPOLLOUT)
        {
            std::cout << currentFd << "EPOLL_OUT" << std::endl;
            if (!clientResponse(epoll, this->client[currentFd]))
                std::cerr << "clientResponse Error" << std::endl;
        }
    }
    return RET_OK;
}

RetStatus Server::cgiEventLoop(Epoll &epoll, Client *pipeClient, FD currentFd, u_int32_t currentEvent)
{
    if (currentFd == pipeClient->getPipeFd(InFlag) && (currentEvent & EPOLLHUP))
    {
        epollGuard(epoll, EPOLL_CTL_DEL, currentFd, 0, pipeClient);
        this->pipeToClientMap.erase(currentFd);
        pipeClient->pipeClose(InFlag);
        return RET_OK;
    }
    if (currentEvent & EPOLLERR)
    {
        epollGuard(epoll, EPOLL_CTL_DEL, currentFd, 0, pipeClient);
        deleteClient(pipeClient->getSocket().getFd());
        return RET_ERROR;
    }
    if (currentEvent & EPOLLIN || currentEvent & EPOLLHUP)
    {
        std::cout << "pipe Read in" << std::endl;
        if (!cgiPipeRead(epoll, pipeClient))
        {
            reapCgiChild(pipeClient->getPid());
            pipeClient->setRunCgi(false);
            pipeClient->setStatusCode(STATUS_BAD_GATEWAY);
            if (!epollGuard(epoll, EPOLL_CTL_MOD, pipeClient->getSocket().getFd(), EPOLLOUT, pipeClient))
            {
                deleteClient(pipeClient->getSocket().getFd());
                return RET_ERROR;
            }
        }
    }
    if (currentEvent & EPOLLOUT)
    {
        std::cout << "pipe Write in" << std::endl;
        if (!cgiPipeWrite(epoll, pipeClient))
        {
            reapCgiChild(pipeClient->getPid());
            pipeClient->setRunCgi(false);
            pipeClient->setStatusCode(STATUS_BAD_GATEWAY);
            if (!epollGuard(epoll, EPOLL_CTL_MOD, pipeClient->getSocket().getFd(), EPOLLOUT, pipeClient))
            {
                deleteClient(pipeClient->getSocket().getFd());
                return RET_ERROR;
            }
        }
    }
    return RET_OK;
}

RetStatus Server::clientResponse(Epoll &epoll, Client *client)
{
    std::string response;

    client->timeSet(this->timeOutValue.writeTimeout);

    Response res;
    if (client->getRequest().status != STATUS_UNDEFINED)
    {
        res = Response(client->getRequest().status);
    }
    else
    {
        const RouteResult &route = client->getRouteResult();

        switch (route.action)
        {
            case ACTION_STATIC:
                res = Handler::serve(route, client->getRequest());
                break;
            case ACTION_REDIRECT:
                res = Response(static_cast<Status>(route.redirectCode));
                res.headers["Location"] = route.redirectPath;
                break;
            case ACTION_CGI:
                if (client->getStatusCode() != 0)
                    res = Response(static_cast<Status>(client->getStatusCode()));
                else
                    res = client->getCgiResponse();
                break;
            case ACTION_ERROR:
            default:
                res = Response(static_cast<Status>(route.errorCode));
                if (route.errorCode == STATUS_METHOD_NOT_ALLOWED)
                {
                    std::string allow;
                    for (std::set<HttpMethod>::const_iterator it = route.allowedMethods.begin(); it != route.allowedMethods.end(); ++it)
                    {
                        if (!allow.empty())
                            allow += ", ";
                        allow += HttpUtils::getMethodName(*it);
                    }
                    res.headers["Allow"] = allow;
                }
                break;
        }
    }

    // 에러 상태인데 body가 비어있으면(라우팅/CGI/파싱 에러 등) errorPages 설정을 확인해
    // 커스텀 에러 페이지로, 없으면 webserv 기본 에러 페이지로 body를 채운다.
    if (static_cast<int>(res.statusCode) >= 400 && res.body.empty())
    {
        ServerConfig &config = this->configs[client->getListenFd()];
        Response errPage = Handler::buildErrorPage(res.statusCode, config.getErrorPages());
        for (std::map<std::string, std::string>::const_iterator it = res.headers.begin(); it != res.headers.end(); ++it)
            errPage.headers[it->first] = it->second;
        res = errPage;
    }
    response = res.toString(client->getShouldClose());

    client->response = response;
    int sendStatus = serverSend(epoll, client);
    if (sendStatus == RET_ERROR)
    {
        deleteClient(client->getSocket().getFd());
        return RET_ERROR;
    }
    if (sendStatus == RET_RE) {return RET_OK;}
    if (client->getShouldClose())
    {
        std::cout << "클라이언트 연결 종료 : Client["<< client->getSocket().getFd() << "]" << std::endl;
        epollGuard(epoll, EPOLL_CTL_DEL, client->getSocket().getFd(), 0, client);
        deleteClient(client->getSocket().getFd());
        return RET_OK;
    }
    client->timeSet(this->timeOutValue.keepAliveTimeout);
    return RET_OK;
}

/*
 * @todo 나중에 response가 vector로 변경되면 erase를 하는 로직 추가
 * @todo send한 내용의 길이를 response의 총 길이와 비교하는 로직 추가(if문 분기는 작성완료, length를 더하는 로직 작성)
 */
RetStatus Server::serverSend(Epoll &epoll, Client *client)
{
    ssize_t targetSize = client->response.size();
    ssize_t length = send(client->getSocket().getFd(), client->response.c_str(), targetSize, 0);
    if (length < 0)
        return RET_ERROR;
    if (length == targetSize)
    {
        if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLIN, client))
            return (RET_ERROR);
        client->resetForNextRequest();
        return (RET_OK);
    }
    std::cout << "클라이언트 연결 유지 : Client["<< client->getSocket().getFd() << "]" << std::endl;
    client->response = client->response.substr(length);
    return (RET_RE);
}

/**
 * @brief 서버 소켓의 바인딩 및 리슨을 설정하는 함수
 *
 * TIME_WAIT 방지를 위해 SO_REUSEADDR를 설정하고 커널에 지정된 포트로 bind를 요청한 후, 클라이언트의 연결을 큐에 쌓기 시작하는 listen()과 논블로킹 설정을 호출합니다.
 * @param serverSocket 설정할 서버 Socket 객체
 * @return Error 발생시 0, 정상 동작시 1반환 (현재 enum을 통해서 type.hpp에 정의)
 */
RetStatus Server::serverSetting(Socket *serverSocket)
{
    int flag = 1;
    setsockopt(serverSocket->getFd(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (bind(serverSocket->getFd(), reinterpret_cast<const sockaddr *>(&serverSocket->getAddr()), sizeof(serverSocket->getAddr())) < 0)
    {
        delete serverSocket;
        return RET_ERROR;
    }
    if (listen(serverSocket->getFd(), SOMAXCONN) < 0)
    {
        delete serverSocket;
        return RET_ERROR;
    }
    nonblockingSet(serverSocket->getFd());
    return RET_OK;
}

RetStatus Server::clientAccept(Epoll &epoll, Socket *socket)
{
    int tmpFd = 0;
    Socket *tmpSocket;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    tmpFd = accept(socket->getFd(), (sockaddr *)&clientAddr, &clientLen);
    if (tmpFd < 0)
    {
        std::cerr << "Client Accept Failed" << std::endl;
        return RET_ERROR;
    }
    if (!nonblockingSet(tmpFd))
    {
        close(tmpFd);
        return RET_ERROR;
    }
    if (!(tmpSocket = new Socket(tmpFd, clientAddr)))
    {
        std::cerr << "Client Accept Failed" << std::endl;
        return RET_ERROR;
    }
    tmpSocket->setTimeStatus(this->timeOutValue.connectionTimeOut);
    std::cout << "Client " << tmpFd <<"(" << tmpSocket->getAddr().sin_addr.s_addr << ") port " << tmpSocket->getAddr().sin_port << " " << std::endl;
    if (tmpFd >= 8192)
    {
        Client *client = new Client(tmpSocket, this->env);
        Response res(STATUS_SERVICE_UNAVAILABLE);
        client->response = res.toString(true);
        serverSend(epoll, client);
        delete client;
        return RET_ERROR;
    }
    this->client[tmpFd] = new Client(tmpSocket, this->env);
    this->client[tmpFd]->setListenFd(socket->getFd());
    this->inClientVec.push_back(tmpFd);
    if (!epollGuard(epoll, EPOLL_CTL_ADD, tmpFd, EPOLLIN, this->client[tmpFd]))
    {
        this->deleteClient(tmpFd);
        return RET_ERROR;
    }
    return RET_OK;
}

/**
 * @todo http요청을 다 받은 후에 flag가 넘어오면 그때 EPOLLOUT을 감지하도록 변경
 */
RetStatus Server::clientRequest(Epoll &epoll, Client *client)
{
    unsigned char received[4096];
    int length = recv(client->getSocket().getFd(), received, sizeof(received) -1, 0);
    if (client->getCharDq().empty())
        client->getSocket().setTimeStatus(this->timeOutValue.readTimeout);
    ServerConfig &config = this->configs[client->getListenFd()];
    if (length < 0)
    {
        epollGuard(epoll, EPOLL_CTL_DEL, client->getSocket().getFd(), 0, client);
        this->deleteClient(client->getSocket().getFd());
        return RET_ERROR;
    }
    else if (length == 0)
    {
        std::cout << "클라이언트 정상 종료 : Client["<< client->getSocket().getFd() << "]" << std::endl;
        epollGuard(epoll, EPOLL_CTL_DEL, client->getSocket().getFd(), 0, client);
        this->deleteClient(client->getSocket().getFd());
        return RET_OK;
    }
    else
    {
        received[length] = '\0';
        std::cout << "클라이언트 연결 : Client["<< client->getSocket().getFd() << "]" << std::endl;
        client->CharDqAppend(length, received);
        client->setMaxBodyLength(config.getClientMaxBodySize());
        ReqParseResult ret = client->onReceive();
        if (ret == REQ_PARSE_INCOMPLETE)
            return (RET_RE);
    }
    if (client->getRequest().status == STATUS_UNDEFINED)
    {
        RouteResult route = Router::route(config, client->getRequest());
        if (route.action == ACTION_CGI && !client->getRunCgi())
        {
            bool notFound = false;
            if (!client->checkRunCgi(config.matchLocation, route.resolvedPath, notFound))
            {
                int errorCode = notFound ? STATUS_NOT_FOUND : STATUS_FORBIDDEN;
                return errorHandling(client, epoll, errorCode);
            }
            client->setRouteResult(route);
            if (!cgiRun(epoll, client))
                return errorHandling(client, epoll, STATUS_INTERNAL_SERVER_ERROR);
            return RET_OK;
        }
        client->setRouteResult(route);
    }
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
            return errorHandling(client, epoll, STATUS_INTERNAL_SERVER_ERROR);
    return RET_OK;
}

RetStatus Server::cgiRun(Epoll &epoll, Client *client)
{
    ServerConfig &serverConfig = this->configs[client->getListenFd()];
    Cgi cgi(serverConfig.getMatchedPrefix(), serverConfig.matchLocation);
    pid_t tmpPid;
    Pipe &pipe = client->getCgiPipe();
    int eventSocket = client->getSocket().getFd();

    if (!pipe.init())
        return RET_ERROR;

    // fork는 4개 fd 모두 유효한 상태에서 먼저 실행
    tmpPid = cgi.excute(client, this->env, pipe.getInPipeArr(), pipe.getOutPipeArr());
    if (static_cast<int>(tmpPid) < 0)
    {
        pipe.detach();  // excute가 fork 실패 시 내부에서 이미 close함
        return RET_ERROR;
    }
    pipe.closeChildSide();
    FD inWriteFd = pipe.getInWriteFd();
    FD outReadFd = pipe.getOutReadFd();

    if (!epollGuard(epoll, EPOLL_CTL_MOD, eventSocket, 0, client))
    {
        cgiRollback(client, tmpPid);
        return RET_ERROR;
    }
    if (!epollGuard(epoll, EPOLL_CTL_ADD, inWriteFd, EPOLLOUT, client))
    {
        cgiRollback(client, tmpPid);
        return RET_ERROR;
    }
    if (!epollGuard(epoll, EPOLL_CTL_ADD, outReadFd, EPOLLIN, client))
    {
        cgiRollback(client, tmpPid);
        return RET_ERROR;
    }
    client->setPid(tmpPid);
    client->setRunCgi(true);
    client->getSocket().setTimeStatus(this->timeOutValue.cgiTimeout);
    this->pipeToClientMap[inWriteFd] = eventSocket;
    this->pipeToClientMap[outReadFd] = eventSocket;
    return RET_OK;
}

/**
 * @todo cgi가 분기되어 favicon요청 안 들어오면 그거에 마춰서 코드 수정해야함 (pipe가 잘 닫히는지 확인 및 강제로 read 한번 더 호출해서 강제로 pipe를 제거하는 로직 제거 및 오류 확인)
 */
RetStatus Server::cgiPipeRead(Epoll &epoll, Client *client)
{
    std::cout << "Pipe Read : Client[" << client->getSocket().getFd() << "]" << std::endl;
    int status = client->readCgiPipe();
    if (status == RET_ERROR) //cgi문제로 인한 오류 routing
    {
        client->setStatusCode(STATUS_BAD_GATEWAY);
        client->clearCgiRawOutput();
        FD outFd = client->getPipeFd(OutFlag);
        epollGuard(epoll, EPOLL_CTL_DEL, outFd, EPOLLIN, client);
        this->pipeToClientMap.erase(outFd);
        client->pipeClose(OutFlag);
        return RET_ERROR;
    }
    else if (status == RET_RE) {return RET_RE;}
    else if (status == RET_OK)
    {
        epollGuard(epoll, EPOLL_CTL_DEL, client->getPipeFd(OutFlag), EPOLLIN, client);
        this->pipeToClientMap.erase(client->getPipeFd(OutFlag));
        client->pipeClose(OutFlag);
        client->setRunCgi(false);
        return epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client);
    }
    return RET_OK;
}

RetStatus Server::cgiPipeWrite(Epoll &epoll, Client *client)
{
    std::cout << "Pipe Write : Client[" << client->getSocket().getFd() << "]" << std::endl;
    int status = client->writeCgiPipe();
    if (status == RET_ERROR)
    {
        client->setStatusCode(STATUS_BAD_GATEWAY);
        client->clearCgiRawOutput();
        epollGuard(epoll, EPOLL_CTL_DEL, client->getPipeFd(InFlag), EPOLLOUT, client);
        this->pipeToClientMap.erase(client->getPipeFd(InFlag));
        client->pipeClose(InFlag);
        return RET_ERROR;
    }
    else if (status == RET_RE) {return RET_OK;}
    else
    {
        epollGuard(epoll, EPOLL_CTL_DEL, client->getPipeFd(InFlag), EPOLLOUT, client);
        this->pipeToClientMap.erase(client->getPipeFd(InFlag));
        client->pipeClose(InFlag);
        return RET_OK;
    }
}

void Server::checkTimeOutClient(Epoll &epoll, int &index)
{
    int i = 0;
    int numClient = static_cast<int>(this->inClientVec.size());
    while (i < 16)
    {
        if (index >= numClient)
            break;
        FD fd = this->inClientVec[index];
        if (this->client[fd] == NULL)
        {
            this->inClientVec.erase(this->inClientVec.begin() + index);
            numClient = static_cast<int>(this->inClientVec.size());
        }
        else if (this->client[fd]->checkAlive())
        {
            if (this->client[fd]->getRunCgi() && cgiTimeoutAbort(epoll, this->client[fd]))
            {
                std::cout << "cgi timeout abort [" << fd << "]" << std::endl;
                index++;
            }
            else if (!this->client[fd]->response.empty())
            {
                // write_timeout: 이미 응답을 보내다 멈춘 클라이언트는 쓰기 방향 자체가 막힌 것이므로
                // 에러 페이지를 새로 만들어 봐야 전달되지 않는다. 재시도 없이 바로 종료한다.
                std::cout << "write timeout delete [" << fd << "]" << std::endl;
                epollGuard(epoll, EPOLL_CTL_DEL, fd, 0, this->client[fd]);
                deleteClient(fd);
                numClient = static_cast<int>(this->inClientVec.size());
            }
            else if (readTimeoutAbort(epoll, this->client[fd]))
            {
                std::cout << "read timeout abort [" << fd << "]" << std::endl;
                index++;
            }
            else
            {
                std::cout << "timeout delete [" << fd << "]" << std::endl;
                epollGuard(epoll, EPOLL_CTL_DEL, fd, 0, this->client[fd]);
                deleteClient(fd);
                numClient = static_cast<int>(this->inClientVec.size());
            }
        }
        else
            index++;
        i++;
    }
    if (index >= numClient)
        index = 0;
}

void Server::deleteClient(int deleteFd)
{
    if (!clientExist(deleteFd))
        return;
    Client *client = this->client[deleteFd];
    pid_t pid = client->getPid();
    if (client->getRunCgi() && waitpid(pid, NULL, WNOHANG) == 0)
    {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    if (this->client[deleteFd]->getPipeFd(InFlag) != -1)
        this->pipeToClientMap.erase(this->client[deleteFd]->getPipeFd(InFlag));
    if (this->client[deleteFd]->getPipeFd(OutFlag) != -1)
        this->pipeToClientMap.erase(this->client[deleteFd]->getPipeFd(OutFlag));
    FdVec::iterator it = std::find(this->inClientVec.begin(), this->inClientVec.end(), deleteFd);
    if (it != this->inClientVec.end())
        this->inClientVec.erase(it);
    delete this->client[deleteFd];
    this->client[deleteFd] = NULL;
}

void Server::reapCgiChild(pid_t pid)
{
    if (waitpid(pid, NULL, WNOHANG) == 0)
    {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
}

bool Server::clientExist(int fd)
{
    if (fd < 0 || static_cast<size_t>(fd) >= this->client.size()) {return false;}
    return this->client[fd] != NULL;
}

RetStatus Server::errorHandling(Client *client, Epoll &epoll, int statusCode)
{
    client->setStatusCode(statusCode);
    RouteResult route = client->getRouteResult();
    route.action = ACTION_ERROR;
    route.errorCode = statusCode;
    route.allowedMethods.clear();
    client->setRouteResult(route);
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
        deleteClient(client->getSocket().getFd());
    return RET_ERROR;
}

RetStatus Server::epollGuard(Epoll &epoll, int op, FD fd, u_int32_t event, Client *client)
{
    if (epoll.epollControl(op, fd, event))
        return RET_OK;
    std::cerr << "epoll_ctl 실패 FD: " << fd << " (Client[" << client->getSocket().getFd() << "])" << std::endl;
    return RET_ERROR;
}

void Server::cgiRollback(Client *client, pid_t pid)
{
    this->reapCgiChild(pid);
    client->pipeClose(InFlag);
    client->pipeClose(OutFlag);
}

RetStatus Server::cgiTimeoutAbort(Epoll &epoll, Client *client)
{
    this->reapCgiChild(client->getPid());
    if (client->getPipeFd(InFlag) != -1)
    {
        epollGuard(epoll, EPOLL_CTL_DEL, client->getPipeFd(InFlag), 0, client);
        this->pipeToClientMap.erase(client->getPipeFd(InFlag));
        client->pipeClose(InFlag);
    }
    if (client->getPipeFd(OutFlag) != -1)
    {
        epollGuard(epoll, EPOLL_CTL_DEL, client->getPipeFd(OutFlag), 0, client);
        this->pipeToClientMap.erase(client->getPipeFd(OutFlag));
        client->pipeClose(OutFlag);
    }
    client->setRunCgi(false);
    client->setRequestStatus(STATUS_GATEWAY_TIMEOUT);
    client->clearCgiRawOutput();
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
        return RET_ERROR;
    return RET_OK;
}

RetStatus Server::readTimeoutAbort(Epoll &epoll, Client *client)
{
    client->setRequestStatus(STATUS_REQUEST_TIMEOUT);
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
        return RET_ERROR;
    client->timeSet(this->timeOutValue.keepAliveTimeout);
    return RET_OK;
}

void Server::serverClose()
{
    this->serverActive = false;
}
