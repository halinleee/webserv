#include "Server.hpp"
#include "main.hpp"

Server::Server(char **envp, timeValue timeValue) : serverActive(true), serverSocket(NULL), client(8192, NULL), env(envpParsing(envp)), timeOutValue(timeValue) {}

Server::~Server()
{
    for (ClientVec::iterator it = this->client.begin(); it != this->client.end(); ++it)
        delete *it;
    delete this->serverSocket;
}

RetStatus Server::serverAdd(in_port_t port, Epoll &epoll)
{
    int socketFd;
    Socket *tmpSocket;
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {return (STATUS_ERROR);}
    tmpSocket = new Socket(socketFd, port);
    if (!serverSetting(tmpSocket)) {return (STATUS_ERROR);}
    if (!epoll.epollControl(EPOLL_CTL_ADD, tmpSocket->getFd(), EPOLLIN))
    {
        delete tmpSocket;
        return (STATUS_ERROR);
    }
    this->serverSocket = tmpSocket;
    return (STATUS_OK);
}

RetStatus Server::eventProcess(Epoll &epoll)
{
    FD currentFd;
    u_int64_t currentEvent;
    int eventCount = 0;
    int index = 0;
    while(serverActive)
    {
        if ((eventCount = epoll.epWait()) < 0) {return (STATUS_ERROR);}
        for (int i = 0; i < eventCount; i++)
        {
            currentFd = epoll[i].data.fd;
            currentEvent = epoll[i].events;
            if (((this->serverSocket->getFd() == currentFd) && (currentEvent & EPOLLIN)) && !clientAccept(epoll, this->serverSocket))
                continue;
            else if (!clientLoop(epoll, currentFd, currentEvent)) 
                continue;
        }
        if (this->inClientVec.size() > 0)
            checkTimeOutClient(index);
    }
    return (STATUS_OK);
}

RetStatus Server::clientLoop(Epoll &epoll, FD currentFd, u_int32_t currentEvent)
{
    if (this->pipeToClientMap.count(currentFd) > 0)
    {
        Client *pipeClient = this->client[this->pipeToClientMap[currentFd]];
        if (currentEvent & EPOLLERR)
        {
            epoll.epollControl(EPOLL_CTL_DEL, currentFd, 0);
            deleteClient(pipeClient->getSocket().getFd());
            return (STATUS_ERROR);
        }
        if (currentEvent & EPOLLIN || currentEvent & EPOLLHUP)
        {
            std::cout << "pipe Read in" << std::endl;
            cgiPipeRead(epoll, pipeClient);
        }
        if (currentEvent & EPOLLOUT)
        {
            std::cout << "pipe Write in" << std::endl;
            cgiPipeWrite(epoll, pipeClient);
        }
        return (STATUS_OK);
    }
    if (clientExist(currentFd))
    {
        if (currentEvent & EPOLLERR || currentEvent & EPOLLHUP)
        {
            epoll.epollControl(EPOLL_CTL_DEL, currentFd, 0);
            deleteClient(currentFd);
            return (STATUS_ERROR);
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
    return (STATUS_OK);
}

/**
 * @todo response 빌더 완성되면 만드는 reponse빌드 부분 로직 추가해서 보내도록 변경해야함
 * @todo recv로 내용을 다 담기 전까지 response가 생성되기 전에 보내지 않고 return하는 로직을 추가해야함
 * @todo timeOut관련해서 로직 추가
 */
RetStatus Server::clientResponse(Epoll &epoll, Client *client)
{
    RetStatus status;
    std::string reciveVecRequest((client->getCharDq().begin()), (client->getCharDq().end()));
    std::string html_body = "<html><body>";
    html_body += "<h1>Received HTTP Request:</h1>";
    html_body += "<pre>" + reciveVecRequest + "</pre>";
    html_body += "</body></html>";
    
    std::stringstream ss;
    ss << html_body.length();
    std::string response = "";
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n"; 
    response += "Content-Length: " + ss.str() + "\r\n";
    response += "\r\n";
    response += html_body;

    if (client->response.empty())
        client->response = response;
    int sendStatus = serverSend(client);
    if (sendStatus == STATUS_ERROR) {return (STATUS_ERROR);}
    if (sendStatus == STATUS_RE) {return (STATUS_OK);}
    epoll.epollControl(EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLIN);
    client->timeSet();
    return (STATUS_OK);
}

/*
 * @todo 나중에 response가 vector로 변경되면 erase를 하는 로직 추가
 * @todo send한 내용의 길이를 response의 총 길이와 비교하는 로직 추가(if문 분기는 작성완료, length를 더하는 로직 작성)
 */
RetStatus Server::serverSend(Client *client)
{
    ssize_t targetSize = client->response.size();
    ssize_t length = send(client->getSocket().getFd(), client->response.c_str(), targetSize, 0);
    if (length < 0)
        return (STATUS_ERROR);
    if (length == targetSize)
    {
        client->response.clear();
        client->getCharDq().clear();
        return (STATUS_OK);
    }
    client->response = client->response.substr(length);
    client->getCharDq().clear();
    return (STATUS_RE);
}

RetStatus Server::serverSetting(Socket *serverSocket)
{
    int flag = 1;
    setsockopt(serverSocket->getFd(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (bind(serverSocket->getFd(), reinterpret_cast<const sockaddr *>(&serverSocket->getAddr()), sizeof(serverSocket->getAddr())) < 0)
    {
        delete serverSocket;
        return (STATUS_ERROR);
    }
    if (listen(serverSocket->getFd(), SOMAXCONN) < 0)
    {
        delete serverSocket;
        return (STATUS_ERROR);
    }
    nonblockingSet(serverSocket->getFd());
    return (STATUS_OK);
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
        return (STATUS_ERROR);
    }
    if (!nonblockingSet(tmpFd))
        return (STATUS_ERROR);
    if (!(tmpSocket = new Socket(tmpFd, clientAddr)))
    {
        std::cerr << "Client Accept Failed" << std::endl;
        return (STATUS_ERROR);
    }
    tmpSocket->setTimeStatus(this->timeOutValue.connetionTimeOut);
    std::cout << "Client " << tmpFd <<"(" << tmpSocket->getAddr().sin_addr.s_addr << ") port " << tmpSocket->getAddr().sin_port << " " << std::endl;
    if (tmpFd > 8192)
    {
        Client *client = new Client(tmpSocket, this->env);
        client->setStatusCode(503);
        serverSend(client);
        return (STATUS_ERROR);
    }
    this->client[tmpFd] = new Client(tmpSocket, this->env);
    this->inClientVec.push_back(tmpFd);
    if (!(epoll.epollControl(EPOLL_CTL_ADD, tmpFd, EPOLLIN)))
    {
        std::cerr << "epoll ADD failed for FD: " << tmpFd << std::endl;
        // response빌드 부분 추가
        serverSend(this->client[tmpFd]);
    }
    return (STATUS_OK);
}

/**
 * @todo http요청을 다 받은 후에 flag가 넘어오면 그때 EPOLLOUT을 감지하도록 변경
 */
RetStatus Server::clientRequest(Epoll &epoll, Client *client)
{
    unsigned char received[4096];
    int cgiFlag = 1;
    int length = recv(client->getSocket().getFd(), received, sizeof(received) -1, 0);
    client->getSocket().setTimeStatus(this->timeOutValue.readTimeout);
    if (length < 0)
        return (STATUS_ERROR);
    else if (length == 0)
    {
        std::cout << "클라이언트 정상 종료 : Client["<< client->getSocket().getFd() << "]" << std::endl;
        epoll.epollControl(EPOLL_CTL_DEL, client->getSocket().getFd(), 0);
        this->deleteClient(client->getSocket().getFd());
        return (STATUS_OK);
    }
    else 
    {
        received[length] = '\0';
        std::cout << "클라이언트 연결 : Client["<< client->getSocket().getFd() << "]" << std::endl;
        client->CharDqAppend(length, received);
        //파싱부분 들어가는 부분
        if (!epoll.epollControl(EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT))
        {
            client->setStatusCode(500);
            epoll.epollControl(EPOLL_CTL_DEL, client->getSocket().getFd(), EPOLLOUT);
            return (STATUS_ERROR);
        }
    }
    // std::cout << client->getCharDq().size() << std::endl;
    // std::cout << received;
    if (cgiFlag && (!client->checkRunCgi()))
    { // 여기있는 조건문으로 우선 cgi를 킬지 안킬지 하는데 판단하는 조건을 나중에 추가해야함
        if (!cgiRun(epoll, client))
        {
            client->setStatusCode(500);
            epoll.epollControl(EPOLL_CTL_DEL, client->getSocket().getFd(), EPOLLOUT);
            return (STATUS_ERROR);
        }
    }
    return (STATUS_OK);
}

RetStatus Server::cgiRun(Epoll &epoll, Client *client)
{
    Cgi cgi;
    pid_t tmpPid;
    int pipeInFd[2], pipeOutFd[2];
    int eventSocket = client->getSocket().getFd();
    
    if (pipe(pipeInFd) < 0) {return (STATUS_ERROR);}
    if (pipe(pipeOutFd) < 0) {return (STATUS_ERROR);}
    
    fcntl(pipeInFd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeInFd[1], F_SETFD, FD_CLOEXEC);
    fcntl(pipeOutFd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeOutFd[1], F_SETFD, FD_CLOEXEC);

    this->pipeToClientMap[pipeInFd[1]] = eventSocket;
    this->pipeToClientMap[pipeOutFd[0]] = eventSocket;
    nonblockingSet(pipeInFd[1]);
    nonblockingSet(pipeOutFd[0]);
    if (!(epoll.epollControl(EPOLL_CTL_DEL, eventSocket, EPOLLOUT))) {return (STATUS_ERROR);}
    if (!(epoll.epollControl(EPOLL_CTL_ADD, pipeInFd[1], EPOLLOUT))) {return (STATUS_ERROR);}
    if (!(epoll.epollControl(EPOLL_CTL_ADD, pipeOutFd[0], EPOLLIN))) {return (STATUS_ERROR);}
    if (static_cast<int>(tmpPid = cgi.excute(this->env, pipeInFd, pipeOutFd)) < 0) {return (STATUS_ERROR);}
    else
    {
        std::cout << "cgi in" << std::endl;
        client->getSocket().setTimeStatus(this->timeOutValue.cgiTimeout);
        client->setRunCgi();
        client->setPid(tmpPid);
        client->setPipeFd(pipeInFd, pipeOutFd);
    }
    return (STATUS_OK);
}

/**
 * @todo cgi가 분기되어 favicon요청 안 들어오면 그거에 마춰서 코드 수정해야함 (pipe가 잘 닫히는지 확인 및 강제로 read 한번 더 호출해서 강제로 pipe를 제거하는 로직 제거 및 오류 확인)
 */
RetStatus Server::cgiPipeRead(Epoll &epoll, Client *client)
{
    
    std::cout << "Pipe Read : Client[" << client->getSocket().getFd() << "]" << std::endl;
    
    int status = client->readCgiPipe();
    if (status == STATUS_ERROR) //cgi문제로 인한 오류 routing
    {
        epoll.epollControl(EPOLL_CTL_DEL, client->getPipeFd(OutFlag), EPOLLIN);
        this->pipeToClientMap.erase(client->getPipeFd(OutFlag));
        client->pipeClose(OutFlag);
        return (STATUS_ERROR);
    }
    else if (status == STATUS_RE) {return (STATUS_RE);}
    else if (status == STATUS_OK)
    {
        epoll.epollControl(EPOLL_CTL_DEL, client->getPipeFd(OutFlag), EPOLLIN);
        this->pipeToClientMap.erase(client->getPipeFd(OutFlag));
        client->pipeClose(OutFlag);
        if (!epoll.epollControl(EPOLL_CTL_ADD, client->getSocket().getFd(), EPOLLOUT))
        {
            std::cout << "EPOLL ERROR!!" << std::endl;
            return (STATUS_ERROR);
        }
        return (STATUS_OK);
    }
    return (STATUS_OK);
}

RetStatus Server::cgiPipeWrite(Epoll &epoll, Client *client)
{
    std::cout << "Pipe Write : Client[" << client->getSocket().getFd() << "]" << std::endl;
    int status = client->writeCgiPipe();
    if (status == STATUS_ERROR) // response 빌더 완성되면 에러 발생시 바로 response 보내기
    {
        epoll.epollControl(EPOLL_CTL_DEL, client->getPipeFd(InFlag), EPOLLOUT);
        this->pipeToClientMap.erase(client->getPipeFd(InFlag));
        client->pipeClose(InFlag);
        return (STATUS_ERROR);
    }
    else if (status == STATUS_RE) {return (STATUS_OK);}
    else
    {
        epoll.epollControl(EPOLL_CTL_DEL, client->getPipeFd(InFlag), EPOLLOUT);
        this->pipeToClientMap.erase(client->getPipeFd(InFlag));
        client->pipeClose(InFlag);
        return (STATUS_OK);
    }
}

void Server::checkTimeOutClient(int &index)
{
    int i = 0;
    int numClient = static_cast<int>(this->inClientVec.size());
    while (i < 16)
    {
        if (index >= numClient)
            break;
        FD fd = this->inClientVec[index];
        bool shouldDelete = (this->client[fd] == NULL);
        if (!shouldDelete && !this->client[fd]->checkAlive())
        {
            std::cout << "timeout delete [" << fd << "]" << std::endl;
            deleteClient(fd);
            shouldDelete = true;
        }
        if (shouldDelete)
        {
            this->inClientVec.erase(this->inClientVec.begin() + index);
            numClient = static_cast<int>(this->inClientVec.size());
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
    Client *client = this->client[deleteFd];
    pid_t pid = client->getPid();
    if (!clientExist(deleteFd))
        return;
    if (client->checkRunCgi() && waitpid(pid, NULL, WNOHANG) == 0)
    {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    if (this->client[deleteFd]->getPipeFd(InFlag) != -1)
        this->pipeToClientMap.erase(this->client[deleteFd]->getPipeFd(InFlag));
    if (this->client[deleteFd]->getPipeFd(OutFlag) != -1)
        this->pipeToClientMap.erase(this->client[deleteFd]->getPipeFd(OutFlag));
    this->inClientVec.erase(this->inClientVec.begin() + (deleteFd));
    delete this->client[deleteFd];
    this->client[deleteFd] = NULL;
}

bool Server::clientExist(int fd)
{
    if (fd < 0 || static_cast<size_t>(fd) >= this->client.size()) {return (false);}
    return (this->client[fd] != NULL);
}

void Server::serverClose()
{
    this->serverActive = false;
}
