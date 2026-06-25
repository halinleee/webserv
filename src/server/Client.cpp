#include "Client.hpp"

Client::Client()
{
    this->clientSocket = 0;
    this->statusCode = 0;
    this->runCgi = false;
    this->pid = -1;
}

Client::Client(Socket *socket, EnvMap env)
{
    this->clientSocket = socket;
    this->statusCode = 0;
    this->env = env;
    this->runCgi = false;
    this->pid = -1;
}

Client::~Client()
{
    delete this->clientSocket;
    // cgiPipe 소멸자가 열려있는 fd를 자동으로 닫음
}

int Client::writeCgiPipe()
{
    ssize_t written = 0;

    if (this->body.empty())
        return STATUS_OK;
    written = write(this->cgiPipe.getInWriteFd(), &this->body[0], this->body.size());
    if (written < 0)
        return STATUS_ERROR;
    this->body.erase(this->body.begin(), this->body.begin() + written);
    if (this->body.empty())
        return STATUS_OK;
    return STATUS_RE;
}

int Client::readCgiPipe()
{
    char received[4096];
    ssize_t length = read(this->cgiPipe.getOutReadFd(), received, 4095);
    if (length < 0)
        return STATUS_ERROR;
    if (length == 0)
        return this->checkCgiExited();
    received[length] = '\0';
    this->response.append(received, length);
    return STATUS_RE;
}

void Client::CgiExited()
{
    kill(this->pid, SIGKILL);
    waitpid(this->pid, NULL, 0);
    std::cout << "cgi timeOut kill" << std::endl;
}

RetStatus Client::checkCgiExited(void)
{
    int status;

    int result = waitpid(this->pid, &status, WNOHANG);
    if (result == 0)
        return STATUS_RE;
    if (result < 0)
        return STATUS_ERROR;
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == 0)
            return STATUS_OK;
        std::cout << "cgi exited with code " << WEXITSTATUS(status) << std::endl;
        return STATUS_ERROR;
    }
    if (WIFSIGNALED(status))
    {
        std::cout << "cgi killed by signal " << WTERMSIG(status) << std::endl;
        return STATUS_ERROR;
    }
    return STATUS_ERROR;
}

void Client::CharDqAppend(int length, unsigned char *received)
{
    this->recDq.insert(this->recDq.end(), received, received + length);
}

void Client::pipeClose(int flag)
{
    if (flag == InFlag)
        this->cgiPipe.closeInWrite();
    else if (flag == OutFlag)
        this->cgiPipe.closeOutRead();
}

int Client::getPipeFd(int index)
{
    if (index == InFlag)
        return this->cgiPipe.getInWriteFd();
    return this->cgiPipe.getOutReadFd();
}

Pipe &Client::getCgiPipe() { return this->cgiPipe; }

pid_t Client::getPid() { return this->pid; }

CharDq &Client::getCharDq(void) { return this->recDq; }

Socket &Client::getSocket() { return *this->clientSocket; }

int Client::getStatusCode() { return this->statusCode; }

bool Client::checkRunCgi(void) { return this->runCgi; }

void Client::setRunCgi(bool value) { this->runCgi = value; }

void Client::setStatusCode(int statusCode) { this->statusCode = statusCode; }

void Client::setPid(pid_t pid) { this->pid = pid; }

bool Client::checkAlive(void) { return this->getSocket().checkTimeOut(); }

void Client::timeSet(time_t addTime) { this->clientSocket->setTimeStatus(addTime); }
