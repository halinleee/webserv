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

void Client::charQueAppend(int length, char *received)
{
    this->recQue.insert(this->recQue.end(), received, received + length);
}

CharQue &Client::getCharQue(void)
{
    return (this->recQue);
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