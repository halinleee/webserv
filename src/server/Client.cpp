#include "Client.hpp"
#include <cerrno>

Client::Client() 
{
    this->clientSocket = 0;
    this->statusCode = 0;
    this->inPipe[0] = -1;
    this->inPipe[1] = -1;
    this->outPipe[0] = -1;
    this->outPipe[1] = -1;
    this->runCgi = false;
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
    this->runCgi = false;
    this->pid = -1;
}

int Client::writeCgiPipe()
{
    ssize_t written = 0;

    if (this->body.empty())
        return (STATUS_OK);
    written = write(this->inPipe[1], &this->body[0], this->body.size());
    if (written <= 0)
        return (STATUS_ERROR);
    this->body.erase(this->body.begin(), this->body.begin() + written);
    if (this->body.empty())
        return (STATUS_OK);
    else
        return (STATUS_RE);
}

int Client::readCgiPipe()
{
    char received[4096];
    ssize_t length = read(this->outPipe[0], received, 4095);
    if (length < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return (STATUS_RE);
        return (STATUS_ERROR);
    }
    if (length == 0)
        return (STATUS_OK);
    received[length] = '\0';
    this->response.append(received, length);
    return (STATUS_RE);
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
    if (result < 0) // timeOut에 의한 sigkill 분기
        return (STATUS_ERROR);
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == 0) // 정상종료
            return (STATUS_OK);
        std::cout << "cgi exited with code " << WEXITSTATUS(status) << std::endl;
        return (STATUS_ERROR);
    }
    if (WIFSIGNALED(status)) // 프로세스 돌리고 cmd로 kill의 분기
    {
        std::cout << "cgi killed by signal " << WTERMSIG(status) << std::endl;
        return (STATUS_ERROR);
    }
    return (STATUS_ERROR);
}

bool Client::checkAlive(void)
{
    if (std::time(NULL) >= this->getSocket().getTimeOut())
        return (false);
    else
        return (true);
}

void Client::timeSet(void)
{
    this->getSocket().actTimeSet();
}

void Client::CharDqAppend(int length, unsigned char *received)
{
    this->recDq.insert(this->recDq.end(), received, received + length);
}

void Client::pipeClose(int flag)
{
    if (flag == InFlag)
    {
        if (this->inPipe[0] > -1)
            close(this->inPipe[0]);
        if (this->inPipe[1] > -1)
            close(this->inPipe[1]);
        this->inPipe[0] = -1;
        this->inPipe[1] = -1;
    }
    else if (flag == OutFlag)
    {
        if (this->outPipe[0] > -1)
            close(this->outPipe[0]);
        if (this->outPipe[1] > -1)
            close(this->outPipe[1]);
        this->outPipe[0] = -1;
        this->outPipe[1] = -1;
    }
    
}

void Client::pipeClose(int pipe[2])
{
    if (pipe[0] > -1)
        close(pipe[0]);
    if (pipe[1] > -1)
        close(pipe[1]);
    pipe[0] = -1;
    pipe[1] = -1;
}

void Client::setPipeFd(int inPipe[2], int outPipe[2])
{
    close(inPipe[0]);
    close(outPipe[1]);
    this->inPipe[0] = -1;
    this->inPipe[1] = inPipe[1];
    this->outPipe[0] = outPipe[0];
    this->outPipe[1] = -1;
}

int Client::getPipeFd(int index)
{
    if (index == InFlag)
        return (this->inPipe[1]);
    else
        return (this->outPipe[0]);
}

bool Client::checkAlive(void) { return (this->clientSocket->checkTimeOut());}

pid_t Client::getPid() {return (this->pid);}

CharDq &Client::getCharDq(void) { return (this->recDq);}

Socket &Client::getSocket() { return (*this->clientSocket);}

int Client::getStatusCode() { return (this->statusCode);}

bool Client::checkRunCgi(void) { return (this->runCgi);}

void Client::setRunCgi(void) {this->runCgi ? this->runCgi = false : this->runCgi = true ;}

void Client::setStatusCode(int statusCode) {this->statusCode = statusCode;}

void Client::setPid(pid_t pid) {this->pid = pid;}

void Client::timeSet(time_t addTime) {this->clientSocket->setTimeStatus(addTime); }

Client::~Client()
{
    this->pipeClose(this->inPipe);
    this->pipeClose(this->outPipe);
    delete this->clientSocket;
}