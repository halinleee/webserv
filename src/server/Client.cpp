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
}

void Client::charVecAppend(int length, char *received)
{
    this->recVec.insert(this->recVec.end(), received, received + length);
}

CharVec &Client::getCharVec(void)
{
    return (this->recVec);
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

void Client::delSocket()
{
    // this->pipeClose(this->inPipe);
    // this->pipeClose(this->outPipe);
    delete this->clientSocket;
}

// Client::~Client()
// {
//     delete this->clientSocket;
// }