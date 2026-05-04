#include "Server.hpp"

/**
* @brief Server 생성자. 초기 환경변수를 파싱하여 저장합니다.
* 
* 서버 초기화 단계에서 메모리를 확보하고 envp를 맵 형태로 변환하여 보관합니다.
* @param envp 메인 함수에서 전달받은 환경변수
*/
Server::Server(char **envp) : serverActive(true), serverSocket(NULL), client(8192, NULL), env(envpParsing(envp))
{
}

/**
* @brief Client 맵과 serverSocket으로 할당받은 자원회수
* 
* 서버가 정상 종료되거나 예외로 인해 다운될 때, 연결된 모든 클라이언트와 서버 소켓 자원을 안전하게 해제합니다.
*/
Server::~Server()
{
    for (ClientVec::iterator it = this->client.begin(); it != this->client.end(); ++it)
        delete *it;
    delete this->serverSocket;
}

/**
* @brief 새로운 서버 소켓을 생성하고 Epoll에 등록하는 함수
* 
* 내부적으로 serverSetting()을 호출하여 소켓 옵션을 설정하고 논블로킹 모드로 만든 후, Epoll의 감시 대상에 추가합니다.
* @param port 바인딩할 포트 번호
* @param epoll 이벤트를 관리할 Epoll 객체
* @details 소켓 생성 -> 포트 바인딩/리스닝(serverSetting) -> Epoll에 이벤트(EPOLLIN) 추가라는 서버 준비과정을 단번에 수행합니다.
* @return Error 발생시 0, 정상 동작시 1반환 (현재 enum을 통해서 type.hpp에 정의)
*/
bool Server::serverAdd(in_port_t port, Epoll &epoll)
{
    int socketFd;
    Socket *tmpSocket;
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return (STATUS_ERROR);
    tmpSocket = new Socket(socketFd, port);
    if (!serverSetting(tmpSocket))
        return (STATUS_ERROR);
    if (!epoll.epollControl(EPOLL_CTL_ADD, tmpSocket->getFd(), EPOLLIN))
    {
        delete tmpSocket;
        return (STATUS_ERROR);
    }
    this->serverSocket = tmpSocket;
    return (STATUS_OK);
}

/**
 * @brief Epoll 이벤트를 감지하고 종류에 따라 분기 처리하는 메인 이벤트 루프 함수
 *  
 * 무한 루프 내에서 epWait()을 호출하여 발생하는 이벤트(연결 요청, 데이터 수신, 송신 가능 여부 등)를
 * 판단하고 각각 clientAccept, clientRequest, clientResponse 등으로 라우팅하는 서버의 심장부 역할을 합니다.
 * 
 * 무한 루프 속에서 운영체제에 의해 통지받은 I/O 발생 이벤트들을 해석합니다.
 * 이벤트 FD가 서버소켓이면 새 연결 수락, 일반 소켓이고 읽기 가능하면 요청 수신, 쓰기 가능하면 응답 전송 등 서버의 두뇌 역할을 합니다.
 * @param epoll 이벤트를 관리할 Epoll 객체
 * 
 * 무한 루프 속에서 운영체제에 의해 통지받은 I/O 발생 이벤트들을 해석합니다.
 * 이벤트 FD가 서버소켓이면 새 연결 수락, 일반 소켓이고 읽기 가능하면 요청 수신, 쓰기 가능하면 응답 전송 등 서버의 두뇌 역할을 합니다.
 * 
 * @todo cgi 관련 로직 추가
 * @todo 전체적인 main loop 및 if을 기준으로 리팩토링 필요
 */
bool Server::eventProcess(Epoll &epoll)
{
    FD currentFd;
    u_int64_t currentEvent;
    int eventCount = 0;
    int index = 0;
    while(serverActive)
    {
        if ((eventCount = epoll.epWait()) < 0)
            return (STATUS_ERROR); 
        for (int i = 0; i < eventCount; i++)
        {
            currentFd = epoll[i].data.fd;
            currentEvent = epoll[i].events;
            if ((this->serverSocket->getFd() == currentFd) && (currentEvent & EPOLLIN))
            {
                if (!clientAccept(epoll, this->serverSocket))
                    return (STATUS_ERROR);
            }
            else
            {
                if (!clientLoop(epoll, currentFd, currentEvent))
                    return (STATUS_ERROR);
            }
        }
        if (this->inClientVec.size() > 0)
            checkKeepAliveClient(index);
    }
    return (STATUS_OK);
}

/**
* @brief eventProcess에서 client에 대한 동작(request, response, cgi)에 대한 동작을 수행하는 함수
* @param epoll I/O 이벤트를 관리하는 epoll 객체(오류 발생 및 respose를 보낸 후에 등록했던 이벤트 삭제를 위해 매개변수로 지정)
* @param currentFd 현재 이벤트가 감지된 FD
* @param currentEvent 현재 이벤트의 내용
* @return loop 동작 중 error발생 여부(발생시 STATUS_ERROR = 0, 아닐 시 normalOpration = 1)
*/
bool Server::clientLoop(Epoll &epoll, FD currentFd, u_int32_t currentEvent)
{
    if (this->pipeToClientMap.count(currentFd) > 0)
    {
        Client *pipeClient = this->client[this->pipeToClientMap[currentFd]];
        if (currentEvent & EPOLLERR || currentEvent & EPOLLHUP)
        {
            epoll.epollControl(EPOLL_CTL_DEL, currentFd, 0);
            deleteClient(pipeClient->getSocket().getFd());
            return (STATUS_ERROR);
        }
        if (currentEvent & EPOLLIN)
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
            {
                std::cerr << "clientRequest Error" << std::endl;
                return (STATUS_ERROR);
            }
        }
        if (currentEvent & EPOLLOUT) 
        {
            std::cout << currentFd << "EPOLL_OUT" << std::endl;
            if (!clientResponse(epoll, this->client[currentFd]))
            {
                std::cerr << "clientResponse Error" << std::endl;
                return (STATUS_ERROR);
            }
        }
    }
    return (STATUS_OK);
}

/**
 * @brief 클라이언트에게 응답 데이터를 전송하는 함수
 * 
 * 현재 버퍼에 담겨진 HTTP 요청 내용을 메아리(Echo) 형식의 HTML 응답으로 조립하여 send()로 보내고, 송신 완료 시 epoll 감시 목록에서 삭제 후 클라이언트와의 연결을 끊습니다.
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param client 응답을 보낼 클라이언트 client 객체
 * @todo response 빌더 완성되면 만드는 로직 추가해서 보내도록 변경해야함
 * @todo recv로 내용을 다 담기 전까지 response를 보내지 않고 return하는 로직을 추가해야함
 * @todo keep-alive에 대한 timeAct, timeOut관련해서 로직 추가
 * @todo send한 내용의 길이를 response의 총 길이와 비교하는 로직 추가(if문 분기는 작성완료, length를 더하는 로직 작성)
 */
bool Server::clientResponse(Epoll &epoll, Client *client)
{
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
    {
        std::cout << "normal response send" << std::endl;
        client->response = response;
    }
    else
        std::cout << "cgi response send" << std::endl;
    if (!serverSend(client))
        return (STATUS_OK);
    epoll.epollControl(EPOLL_CTL_DEL, client->getSocket().getFd(), 0);
    return (STATUS_OK);
}

/**
 * @brief 서버에서 client에게 내용을 보내는 함수
 * 
 * client의 response 변수에 있는 response 내용을 보냄(error의 경우 그 상황에 맞춰서 http메세지로 바꿔서 전송하면 됨)
 * @param client 보낼 clinet 객체
 * @return 함수의 성공 여부
 * @todo 나중에 response가 vector로 변경되면 erase를 하는 로직 추가
 */
int Server::serverSend(Client *client)
{
    ssize_t targetSize = 0;
    ssize_t length = 0;

    targetSize = client->response.size();
    length = send(client->getSocket().getFd(), client->response.c_str(), targetSize, 0);
    std::cout << "response print" << std::endl;
    std::cout << client->response << std::endl;
    client->response.clear();
    if (length < 0)
        return (STATUS_ERROR);
    if (static_cast<ssize_t>(length) == targetSize)
        return (STATUS_OK);
    else
    {
        client->response = client->response.substr(length);
        return (STATUS_RE);
    }
}

/**
 * @brief 서버 소켓의 바인딩 및 리슨을 설정하는 함수
 * 
 * TIME_WAIT 방지를 위해 SO_REUSEADDR를 설정하고 커널에 지정된 포트로 bind를 요청한 후, 클라이언트의 연결을 큐에 쌓기 시작하는 listen()과 논블로킹 설정을 호출합니다.
 * @param serverSocket 설정할 서버 Socket 객체
 * @return Error 발생시 0, 정상 동작시 1반환 (현재 enum을 통해서 type.hpp에 정의)
 */
bool Server::serverSetting(Socket *serverSocket)
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

/**
 * @brief 클라이언트의 연결 요청을 수락하고 Client 객체를 생성 및 Epoll에 등록하는 함수
 * 
 * 서버소켓의 EPOLLIN 신호에 따라 accept()를 호출하여 클라이언트의 고유 FD와 IP 정보를 가져옵니다. 이를 바탕으로 Client 객체를 동적 생성해 관리 맵에 등록하고, epoll에 읽기 감시를 요청합니다.
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param socket 연결 요청을 받은 서버 Socket 객체
 * @todo 나중에 error 발생시 반환할 return 코드 추가
 */
bool Server::clientAccept(Epoll &epoll, Socket *socket)
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
    std::cout << "Client " << tmpFd <<"(" << tmpSocket->getAddr().sin_addr.s_addr << ") 포트 " << tmpSocket->getAddr().sin_port << " 를 통해서 접속" << std::endl;
    this->client[tmpFd] = new Client(tmpSocket, this->env);
    this->inClientVec.push_back(tmpFd);
    if (!(epoll.epollControl(EPOLL_CTL_ADD, tmpFd, EPOLLIN)))
    {
        std::cerr << "epoll ADD failed for FD: " << tmpFd << std::endl;
        this->deleteClient(tmpFd);
        return (STATUS_ERROR);
    }
    return (STATUS_OK);
}

/**
 * @brief 클라이언트의 데이터를 읽어들이고 요청을 파싱하는 함수
 * 
 * recv()로 데이터를 받아 Client 객체 내의 수신 버퍼(deque)에 쌓습니다. 만약 데이터가 정상적으로 수신되었다면(연결 유지) 향후 응답을 위해 해당 소켓의 epoll 이벤트를 EPOLLOUT으로 변경합니다.
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param client 데이터를 전송한 클라이언트 객체
 * @todo http요청을 다 받은 후에 flag가 넘어오면 그때 EPOLLOUT을 감지하도록 변경
 */
bool Server::clientRequest(Epoll &epoll, Client *client)
{
    unsigned char received[4096];
    int cgiFlag = 0;
    int length = recv(client->getSocket().getFd(), received, sizeof(received) -1, 0);
    client->timeSet();
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
        if (!epoll.epollControl(EPOLL_CTL_MOD, client->getSocket().getFd(), EPOLLOUT))
        {
            client->setStatusCode(500);
            epoll.epollControl(EPOLL_CTL_DEL, client->getSocket().getFd(), EPOLLOUT);
            this->deleteClient(client->getSocket().getFd());
            return (STATUS_ERROR);
        }
    }
    // std::cout << client->getCharDq().size() << std::endl;
    // std::cout << received;
    if (cgiFlag)
    { // 여기있는 조건문으로 우선 cgi를 킬지 안킬지 하는데 판단하는 조건을 나중에 추가해야함
        if (!cgiRun(epoll, client))
        {
            client->setStatusCode(500);
            epoll.epollControl(EPOLL_CTL_DEL, client->getSocket().getFd(), EPOLLOUT);
            this->deleteClient(client->getSocket().getFd());
            return (STATUS_ERROR);
        }
    }
    // HTTP 내용 파싱
    return (STATUS_OK);
}

/**
 * @brief CGI 프로그램을 실행하고 파이프를 설정하는 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param eventSocket 클라이언트 소켓의 FD
 */
bool Server::cgiRun(Epoll &epoll, Client *client)
{
    Cgi cgi;
    pid_t tmpPid;
    int pipeInFd[2], pipeOutFd[2];
    int eventSocket = client->getSocket().getFd();
    
    if (pipe(pipeInFd) < 0)
        return (STATUS_ERROR);
    if (pipe(pipeOutFd) < 0)
        return (STATUS_ERROR);
    
    fcntl(pipeInFd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeInFd[1], F_SETFD, FD_CLOEXEC);
    fcntl(pipeOutFd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeOutFd[1], F_SETFD, FD_CLOEXEC);
    this->pipeToClientMap[pipeInFd[1]] = eventSocket;
    this->pipeToClientMap[pipeOutFd[0]] = eventSocket;
    nonblockingSet(pipeInFd[1]);
    nonblockingSet(pipeOutFd[0]);
    if (!(epoll.epollControl(EPOLL_CTL_DEL, eventSocket, EPOLLOUT)))
        return (STATUS_ERROR);
    if (!(epoll.epollControl(EPOLL_CTL_ADD, pipeInFd[1], EPOLLOUT)))
        return (STATUS_ERROR);
    if (!(epoll.epollControl(EPOLL_CTL_ADD, pipeOutFd[0], EPOLLIN)))
        return (STATUS_ERROR);
    if (static_cast<int>(tmpPid = cgi.excute(this->env, pipeInFd, pipeOutFd)) < 0)
        return (STATUS_ERROR);
    else
    {
        client->setPid(tmpPid);
        client->setPipeFd(pipeInFd, pipeOutFd);
    }
    return (STATUS_OK);
}

/**
 * @brief CGI 프로세스로부터 데이터를 읽어들이는 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param socket 이벤트가 감지된 pipe의 FD를 가지고 있는 client객체의 주소
 */
bool Server::cgiPipeRead(Epoll &epoll, Client *client)
{
    
    std::cout << "Pipe Read : Client[" << client->getSocket().getFd() << "]" << std::endl;
    
    int status = client->readCgiPipe();
    if (status == STATUS_ERROR)
    {
        epoll.epollControl(EPOLL_CTL_DEL, client->getPipeFd(OutFlag), EPOLLIN);
        this->pipeToClientMap.erase(client->getPipeFd(OutFlag));
        client->pipeClose(OutFlag);
        return (STATUS_ERROR);
    }
    else if (status == STATUS_RE)
        return (STATUS_OK);
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

/**
 * @brief CGI 프로세스로 데이터를 전송(쓰기)하는 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param client 데이터를 송신할 클라이언트의 Socket 객체
 */
bool Server::cgiPipeWrite(Epoll &epoll, Client *client)
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
    else if (status == STATUS_RE)
        return (STATUS_OK);
    else
    {
        epoll.epollControl(EPOLL_CTL_DEL, client->getPipeFd(InFlag), EPOLLOUT);
        this->pipeToClientMap.erase(client->getPipeFd(InFlag));
        client->pipeClose(InFlag);
        return (STATUS_OK);
    }
}

void Server::checkKeepAliveClient(int &index)
{
    int i = 0;
    int numClient = static_cast<int>(this->inClientVec.size()); 
    while (i < 16)
    {
        if (index >= numClient)
            break;
        if (!(this->client[this->inClientVec[index]]->checkAlive()))
        {
            std::cout << "delete [" << this->inClientVec[index] <<"]" << std::endl;
            deleteClient(this->inClientVec[index]);
            this->inClientVec.erase(this->inClientVec.begin() + index);
        }
        index++;
        i++;
    }
    if (index >= numClient)
        index = 0;
    return ;
}

/**
 * @brief 특정 클라이언트의 연결을 종료하고 자원을 해제하는 함수
 * 
 * 클라이언트의 요청이 완료되거나 에러로 인해 연결이 끊겼을 때 호출됩니다. (현재 주석 처리되어 있으나) CGI 파이프 맵 제거 및 프로세스 종료 처리, Client 객체 delete와 컨테이너에서의 제거를 수행합니다.
 * @param deleteFd 삭제할 클라이언트의 소켓 FD
 */
void Server::deleteClient(int deleteFd)
{
    pid_t clientPid = this->client[deleteFd]->getPid();
    if (clientPid > 0)
    {
        if (waitpid(clientPid, NULL, WNOHANG) == 0)
        {
            kill(clientPid, SIGKILL);
            waitpid(clientPid, NULL, 0);
        }
    }
    if (this->client[deleteFd]->getPipeFd(InFlag) != -1)
        this->pipeToClientMap.erase(this->client[deleteFd]->getPipeFd(InFlag));
    if (this->client[deleteFd]->getPipeFd(OutFlag) != -1)
        this->pipeToClientMap.erase(this->client[deleteFd]->getPipeFd(OutFlag));
    if (!clientExist(deleteFd))
        return;
    delete this->client[deleteFd];
    this->client[deleteFd] = NULL;
}

/**
 * @brief 특정 FD에 해당하는 클라이언트 객체의 포인터가 존재하는지 확인하는 함수
 * @param fd 확인할 파일 디스크립터
 * @return 클라이언트 존재 여부 (존재 시 true, 미존재 시 false)
 */
bool Server::clientExist(int fd)
{
    if (fd < 0 || static_cast<size_t>(fd) >= this->client.size())
        return (false);
    return (this->client[fd] != NULL);
}

void Server::serverClose()
{
    this->serverActive = false;
}
