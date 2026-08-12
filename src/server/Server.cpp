#include "Server.hpp"
#include "Pipe.hpp"
#include "Response.hpp"
#include "Router.hpp"
#include "Handler.hpp"

#include <cctype>
#include <csignal>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{
    const size_t DRAIN_MAX_BYTES = 65536;
}

Server::Server() : serverActive(true), client(60000, NULL), env(), timeOutValue() {}

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
            if (!clientRequest(epoll, this->client[currentFd]))
                Logger(LOG_ERROR, "clientRequest Error");
        }
        if (currentEvent & EPOLLOUT)
        {
            if (!clientResponse(epoll, this->client[currentFd]))
                Logger(LOG_ERROR, "clientResponse Error");
        }
    }
    return RET_OK;
}

RetStatus Server::cgiEventLoop(Epoll &epoll, Client *pipeClient, FD currentFd, u_int32_t currentEvent)
{
    if (currentFd == pipeClient->getPipeFd(InFlag) && (currentEvent & EPOLLERR || currentEvent & EPOLLHUP))
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
        if (!cgiPipeRead(epoll, pipeClient))
        {
            reapCgiChild(pipeClient->getPid());
            pipeClient->setRunCgi(false);
            // 요청 자체는 온전히 받은 뒤 upstream(CGI)만 실패한 것이므로 연결은 재사용할 수 있다.
            pipeClient->fail(STATUS_BAD_GATEWAY, FAIL_KEEP_ALIVE);
            if (!epollGuard(epoll, EPOLL_CTL_MOD, pipeClient->getSocket().getFd(), EPOLLOUT, pipeClient))
            {
                deleteClient(pipeClient->getSocket().getFd());
                return RET_ERROR;
            }
        }
    }
    if (currentEvent & EPOLLOUT)
    {
        if (!cgiPipeWrite(epoll, pipeClient))
        {
            reapCgiChild(pipeClient->getPid());
            pipeClient->setRunCgi(false);
            pipeClient->fail(STATUS_BAD_GATEWAY, FAIL_KEEP_ALIVE);
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
    client->timeSet(this->timeOutValue.writeTimeout);

    // client->response가 비어있지 않다면 이전 호출에서 send()가 일부만 전송되어
    // 남은 바이트를 이어 보내야 하는 "이어보내기 모드"이다. 이 경우 라우팅/핸들러/
    // 에러페이지 생성 로직(중복 실행 시 재업로드, 이중 삭제 등 부작용 발생)을 건너뛰고
    // 곧바로 serverSend부터 재시도한다.
    if (client->response.empty())
    {
        std::string response;

        Response res;
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

        // 에러 상태인데 body가 비어있으면(라우팅/CGI/파싱 에러 등) errorPages 설정을 확인해
        // 커스텀 에러 페이지로, 없으면 webserv 기본 에러 페이지로 body를 채운다.
        if (static_cast<int>(res.statusCode) >= 400 && res.body.empty())
        {
            ServerConfig &config = this->configs[client->getListenFd()];
            Response errPage = Handler::buildErrorPage(res.statusCode, config.getErrorPages());
            for (Response::HeaderMap::const_iterator it = res.headers.begin(); it != res.headers.end(); ++it)
            {
                std::string lowerKey = it->first;
                for (size_t i = 0; i < lowerKey.size(); ++i)
                    lowerKey[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowerKey[i])));
                if (lowerKey == "content-type")
                    continue;
                errPage.headers[it->first] = it->second;
            }
            res = errPage;
        }
        response = res.toString(client->getShouldClose());
        Logger(LOG_ACCESS, buildAccessLog(client, res.statusCode, res.body.size()), ipToString(client->getSocket().getAddr().sin_addr.s_addr));
        client->response = response;
    }
    int sendStatus = serverSend(epoll, client);
    if (sendStatus == RET_ERROR)
    {
        deleteClient(client->getSocket().getFd());
        return RET_ERROR;
    }
    if (sendStatus == RET_RE) {return RET_OK;}
    if (client->getShouldClose())
    {
        epollGuard(epoll, EPOLL_CTL_DEL, client->getSocket().getFd(), 0, client);
        drainSocket(client->getSocket().getFd());
        deleteClient(client->getSocket().getFd());
        return RET_OK;
    }
    client->timeSet(this->timeOutValue.keepAliveTimeout);
    return RET_OK;
}

RetStatus Server::serverSend(Epoll &epoll, Client *client)
{
    size_t offset = client->getSentOffset();
    ssize_t remaining = client->response.size() - offset;
    ssize_t length = send(client->getSocket().getFd(), client->response.c_str() + offset, remaining, 0);
    if (length < 0)
        return RET_ERROR;
    if (length == remaining)
    {
        if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLIN, client))
            return (RET_ERROR);
        client->resetForNextRequest();
        return (RET_OK);
    }
    client->addSentOffset(length);
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
        Logger(LOG_ERROR, "Client Accept Failed");
        return RET_ERROR;
    }
    if (!nonblockingSet(tmpFd))
    {
        close(tmpFd);
        return RET_ERROR;
    }
    if (!(tmpSocket = new Socket(tmpFd, clientAddr)))
    {
        Logger(LOG_ERROR, "Client Accept Failed");
        return RET_ERROR;
    }
    tmpSocket->setTimeStatus(this->timeOutValue.connectionTimeOut);
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
        epollGuard(epoll, EPOLL_CTL_DEL, client->getSocket().getFd(), 0, client);
        this->deleteClient(client->getSocket().getFd());
        return RET_OK;
    }
    else
    {
        received[length] = '\0';
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
            int errorCode = STATUS_INTERNAL_SERVER_ERROR;
            if (!client->checkRunCgi(config.matchLocation, route.resolvedPath, errorCode))
                return errorHandling(client, epoll, errorCode, FAIL_KEEP_ALIVE);
            client->setRouteResult(route);
            if (!cgiRun(epoll, client))
                return errorHandling(client, epoll, STATUS_INTERNAL_SERVER_ERROR, FAIL_KEEP_ALIVE);
            return RET_OK;
        }
        client->setRouteResult(route);
    }
    else
        // 파싱이 실패한 요청은 다음 요청의 경계를 신뢰할 수 없으므로 응답 후 연결을 닫는다.
        return errorHandling(client, epoll, client->getRequest().status, FAIL_CLOSE);
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
            return errorHandling(client, epoll, STATUS_INTERNAL_SERVER_ERROR, FAIL_CLOSE);
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

RetStatus Server::cgiPipeRead(Epoll &epoll, Client *client)
{
    int status = client->readCgiPipe();
    if (status == RET_ERROR) //cgi문제로 인한 오류 routing
    {
        // 502 확정은 호출자(cgiEventLoop)가 담당한다. 여기서는 파이프 자원만 정리한다.
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
    int status = client->writeCgiPipe();
    if (status == RET_ERROR)
    {
        // 502 확정은 호출자(cgiEventLoop)가 담당한다. 여기서는 파이프 자원만 정리한다.
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
        else if (this->client[fd]->checkAlive() || checkMemoryLimit(this->client[fd]))
        {
            if (this->client[fd]->getRunCgi() && cgiTimeoutAbort(epoll, this->client[fd]))
                index++;
            else if (!this->client[fd]->response.empty())
            {
                epollGuard(epoll, EPOLL_CTL_DEL, fd, 0, this->client[fd]);
                deleteClient(fd);
                numClient = static_cast<int>(this->inClientVec.size());
            }
            else if (readTimeoutAbort(epoll, this->client[fd]))
                index++;
            else
            {
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

void Server::drainSocket(FD fd)
{
    char buf[4096];
    size_t drained = 0;

    while (drained < DRAIN_MAX_BYTES)
    {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        drained += static_cast<size_t>(n);
    }
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

RetStatus Server::errorHandling(Client *client, Epoll &epoll, int statusCode, FailMode mode)
{
    client->fail(static_cast<Status>(statusCode), mode);
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
        deleteClient(client->getSocket().getFd());
    return RET_ERROR;
}

RetStatus Server::epollGuard(Epoll &epoll, int op, FD fd, u_int32_t event, Client *client)
{
    if (epoll.epollControl(op, fd, event))
        return RET_OK;
    std::ostringstream oss;
    oss << "epoll_ctl 실패 FD: " << fd << " (Client[" << client->getSocket().getFd() << "])";
    Logger(LOG_ERROR, oss.str());
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
    client->clearCgiRawOutput();
    client->fail(STATUS_GATEWAY_TIMEOUT, FAIL_CLOSE);
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
        return RET_ERROR;
    client->timeSet(this->timeOutValue.keepAliveTimeout);
    return RET_OK;
}

RetStatus Server::readTimeoutAbort(Epoll &epoll, Client *client)
{
    client->fail(STATUS_REQUEST_TIMEOUT, FAIL_CLOSE);
    if (!epollGuard(epoll, EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT, client))
        return RET_ERROR;
    client->timeSet(this->timeOutValue.keepAliveTimeout);
    return RET_OK;
}

std::string Server::buildAccessLog(Client *client, Status statusCode, size_t bodySize) const
{
    std::ostringstream line;
    const Request &req = client->getRequest();
    line << "\"" + HttpUtils::getMethodName(client->getRequest().method) + " ";
    line << client->getRequest().path + " ";
    line << "HTTP/1.1\" ";
    line << static_cast<int>(statusCode) << " ";
    line << bodySize << " ";
    
    
    std::map<std::string, std::string>::const_iterator it;
    std::string referer = "-";
    if ((it = req.headers.find("referer")) != req.headers.end())
        referer = it->second;
    std::string userAgent = "-";
    if ((it = req.headers.find("user-agent")) != req.headers.end())
        userAgent = it->second;
    line << "\"" + referer + "\" \"" + userAgent + "\"";
    return line.str();
}

bool Server::checkMemoryLimit(Client *client)
{
    if (!client->getRunCgi())
        return false;
    char buf[4096];
    std::ostringstream path;
    path << "/proc/" << client->getPid() << "/status";
    FD statusFile = open(path.str().c_str(), O_RDONLY);
    if (statusFile < 0)
        return false;
    ssize_t n = read(statusFile, buf, sizeof(buf) - 1);
    close(statusFile);
    if (n <= 0)
        return false;
    buf[n] = '\0';
    std::string statusContent(buf);
    size_t pos = statusContent.find("VmRSS:");
    if (pos == std::string::npos) return false;

    size_t kb = 0;
    std::istringstream(statusContent.substr(pos + 6)) >> kb;
    return kb > 655360;
}

void Server::serverClose()
{
    this->serverActive = false;
}
