#include "Server.hpp"

/**
 * @brief Server 생성자. 초기 환경변수를 파싱하여 저장합니다.
 * @param envp 메인 함수에서 전달받은 환경변수
 */
Server::Server(char **envp)
{
    this->env = envpParsing(envp);
}

/**
 * @brief Server 소멸자
 */
Server::~Server()
{
	for (ClientMap::iterator it = this->client.begin(); it != this->client.end(); ++it)
		delete (it->second);
    delete this->serverSocket;
	client.clear();
}

/**
 * @brief 새로운 서버 소켓을 생성하고 Epoll에 등록하는 함수
 * @param port 바인딩할 포트 번호
 * @param epoll 이벤트를 관리할 Epoll 객체
 */
void Server::serverAdd(in_port_t port, Epoll &epoll)
{
    int socketFd;
    Socket *tmpSocket;
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw (Server::SocketMakeError());
    tmpSocket = new Socket(socketFd, port);
    serverSetting(tmpSocket);
    if (epoll.epCtl(EPOLL_CTL_ADD, tmpSocket->getFd(), EPOLLIN))
        throw (Epoll::EpollCtlError());
    this->serverSocket = tmpSocket;
}

/**
 * @brief Epoll 이벤트를 감지하고 종류에 따라 분기 처리하는 메인 이벤트 루프 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 */
void Server::eventProcess(Epoll &epoll)
{
    int eventCount = 0;
    while(1)
    {
        if (eventCount = epoll.epWait() < 0)
            return ; // respose로 서버 터진거 알려야함(status code 전송?)
        for (int i = 0; i < eventCount; i++)
        {
            if ((this->serverSocket->getFd() == epoll[i].data.fd) && (epoll[i].events & EPOLLIN))
                clientAccept(epoll, this->serverSocket);
            else if (this->client.count(epoll[i].data.fd))
            {
                if (epoll[i].events & EPOLLERR || epoll[i].events & EPOLLHUP)
                {
                    epoll.epCtl(EPOLL_CTL_DEL, epoll[i].data.fd, EPOLLIN | EPOLLOUT);
                    clientDel(epoll[i].data.fd);
                    continue;
                }
                if (epoll[i].events & EPOLLIN) {
                    std::cout << epoll[i].data.fd << "EPOLL_IN" << std::endl;
                    clientRequest(epoll, &(this->client[epoll[i].data.fd]->getSocket()));
                }
                if (this->client.count(epoll[i].data.fd) && (epoll[i].events & EPOLLOUT)) {
                    std::cout << epoll[i].data.fd << "EPOLL_OUT" << std::endl;
                    clientResponse(epoll, &(this->client[epoll[i].data.fd]->getSocket()));
                }
            }
        }
    }
}

/**
 * @brief 클라이언트에게 응답 데이터를 전송하는 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param socket 응답을 보낼 클라이언트 Socket 객체
 */
void Server::clientResponse(Epoll &epoll, Socket *socket)
{
    std::string recive((this->client[socket->getFd()]->getCharVec().begin()), (this->client[socket->getFd()]->getCharVec().end()));

    std::string html_body = "<html><body>";
    html_body += "<h1>Received HTTP Request:</h1>";
    html_body += "<pre>" + recive + "</pre>";
    html_body += "</body></html>";
    
    std::stringstream ss;
    ss << html_body.length();
    std::string response = "";
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n"; 
    response += "Content-Length: " + ss.str() + "\r\n";
    response += "\r\n";
    response += html_body;

    // HTTP 내용 만들기()?
    send(socket->getFd(), response.c_str(), response.size(), 0);
    epoll.epCtl(EPOLL_CTL_DEL, socket->getFd(), EPOLLIN | EPOLLOUT);
    this->clientDel(socket->getFd());
}

/**
 * @brief 서버 소켓의 바인딩 및 리슨을 설정하는 함수
 * @param serverSocket 설정할 서버 Socket 객체
 */
void Server::serverSetting(Socket *serverSocket)
{
    int flag = 1;
    setsockopt(serverSocket->getFd(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (bind(serverSocket->getFd(), reinterpret_cast<const sockaddr *>(&serverSocket->getAddr()), sizeof(serverSocket->getAddr())) < 0)
        throw (Server::SocketMakeError());
    if (listen(serverSocket->getFd(), MAX_INPUT) < 0)
        throw (Server::SocketMakeError());
    nonblockingSet(serverSocket->getFd());
}

/**
 * @brief 클라이언트의 연결 요청을 수락하고 Client 객체를 생성 및 Epoll에 등록하는 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param socket 연결 요청을 받은 서버 Socket 객체
 */
void Server::clientAccept(Epoll &epoll, Socket *socket)
{
    int tmpFd = 0;
    Socket *tmpSocket;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(socket->getAddr());
    
    tmpFd = accept(socket->getFd(), (sockaddr *)&clientAddr, &clientLen);
    if (tmpFd < 0)
    {
        std::cerr << "Client Accept Failed" << std::endl;
        return; // throw 대신 return으로 넘겨서 서버 전체가 죽는 것을 방지
    }
    nonblockingSet(tmpFd);
    tmpSocket = new Socket(tmpFd, clientAddr);
    std::cout << "Client 접속" << tmpFd << std::endl;
    this->client[tmpFd] = new Client(tmpSocket, this->env);
    if (!(epoll.epCtl(EPOLL_CTL_ADD, tmpFd, EPOLLIN)))
    {
        this->client[tmpFd]->setStatusCode(500);
        epoll.epCtl(EPOLL_CTL_DEL, tmpFd, EPOLLIN);
        epoll.epCtl(EPOLL_CTL_ADD, tmpFd, EPOLLOUT);
    }
}

/**
 * @brief 클라이언트의 데이터를 읽어들이고 요청을 파싱하는 함수
 * @param epoll 이벤트를 관리할 Epoll 객체
 * @param socket 데이터를 전송한 클라이언트 Socket 객체
 */
void Server::clientRequest(Epoll &epoll, Socket *socket)
{
    char received[4096];
    int cgiFlag = 0;
    int length = recv(socket->getFd(), received, sizeof(received) -1, 0);
    if (length <= 0)
    {
        std::cout << "클라이언트 연결 종료 : Client["<< socket->getFd() << "]" << std::endl;
        epoll.epCtl(EPOLL_CTL_DEL, socket->getFd(), EPOLLIN | EPOLLOUT);
        this->clientDel(socket->getFd());
        return;
    }
    else 
    {
        received[length] = '\0';
        std::cout << "클라이언트 연결 : Client["<< socket->getFd() << "]" << std::endl;
        this->client[socket->getFd()]->charVecAppend(length, received);
        if (!(epoll.epCtl(EPOLL_CTL_ADD, socket->getFd(), EPOLLOUT)))
        {
            this->client[socket->getFd()]->setStatusCode(500);
            epoll.epCtl(EPOLL_CTL_DEL, socket->getFd(), EPOLLOUT);
            this->clientDel(socket->getFd());
            return ;
        }
    }
    std::cout << client[socket->getFd()]->getCharVec().size() << std::endl;
    std::cout << received;
    if (cgiFlag)
    { // 여기있는 조건문으로 우선 cgi를 킬지 안킬지 하는데 판단하는 조건을 나중에 추가해야함
        if (cgiRun(epoll, socket->getFd()))
        {
            this->client[socket->getFd()]->setStatusCode(500);
            epoll.epCtl(EPOLL_CTL_DEL, socket->getFd(), EPOLLIN | EPOLLOUT);
            this->clientDel(socket->getFd());
            return ;
        }
    }
    // HTTP 내용 파싱
}

/**
 * @brief 특정 파일 디스크립터를 논블로킹(Non-blocking) 모드로 설정하는 함수
 * @param fd 설정할 파일 디스크립터
 */
void Server::nonblockingSet(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief 소켓 생성 실패 시 발생하는 예외 메시지를 반환합니다.
 */
const char *Server::SocketMakeError::what() const throw()
{
    return "socket Make error!!";
}