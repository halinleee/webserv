#include "Client.hpp"

Client::Client() 
{
    this->clientSocket = 0;
    this->statusCode = 0;
    this->inPipe[0] = -1;
    this->inPipe[1] = -1;
    this->outPipe[0] = -1;
    this->outPipe[1] = -1;
    this->pid = -1;
    this->shouldClose = false;
}

Client::Client(Socket *socket, EnvMap env)
{
    this->clientSocket = socket;
    this->statusCode = 0;
    this->env = env;
    this->inPipe[0] = -1;
    this->inPipe[1] = -1;
    this->outPipe[0] = -1;
    this->outPipe[1] = -1;
    this->pid = -1;
    this->shouldClose = false;
}

void Client::CharDqAppend(int length, unsigned char *received)
{
    this->recDq.insert(this->recDq.end(), received, received + length);
}

CharDq &Client::getCharDq(void)
{
    return (this->recDq);
}

Socket &Client::getSocket()
{
    return (*this->clientSocket);
}

int Client::getStatusCode()
{
    return (this->statusCode);
}

void Client::setStatusCode(int statusCode)
{
    this->statusCode = statusCode;
}

// int Client::getPipeFd(int index)
// {
//     if (index == InFlag)
//         return (this->inPipe[1]);
//     else
//         return (this->outPipe[0]);
// }

// void Client::pipeClose(int *pipe)
// {
//     if (pipe[0] > -1)
//         close(pipe[0]);
//     if (pipe[1] > -1)
//         close(pipe[1]);
//     pipe[0] = -1;
//     pipe[1] = -1;
// }

Client::~Client()
{
    // this->pipeClose(this->inPipe);
    // this->pipeClose(this->outPipe);
    delete this->clientSocket;
}

ReqParseResult Client::onReceive()
{
    parser.parse(recDq);
    ReqParseResult ret = parser.getState();
    if (ret == REQ_PARSE_ERROR) shouldClose = true;
    if (ret == REQ_PARSE_INCOMPLETE) return ret;
    request = parser.getRequest();
    parser.clear();
    return ret;

    // REQ_PARSE_DONE   → send 응답 → 정상이면 request.clear() + EPOLLIN 복귀 (TODO)
    // REQ_PARSE_ERROR → send 에러 (Connection: close 포함) → clientDel
    // REQ_PARSE_INCOMPLETE    → EPOLLIN 유지 (데이터 더 기다림)
}

bool Client::getShouldClose() const
{
    return shouldClose;
}