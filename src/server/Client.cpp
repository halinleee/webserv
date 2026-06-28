#include "Client.hpp"

Client::Client()
{
    this->clientSocket = 0;
    this->statusCode = 0;
    this->runCgi = false;
    this->pid = -1;
    this->shouldClose = false;
    this->listenFd = -1;
}

Client::Client(Socket *socket, EnvMap env)
{
    this->clientSocket = socket;
    this->statusCode = 0;
    this->env = env;
    this->runCgi = false;
    this->pid = -1;
    this->shouldClose = false;
    this->listenFd = -1;
}

Client::~Client()
{
    delete this->clientSocket;
    // cgiPipe 소멸자가 열려있는 fd를 자동으로 닫음
}

RetStatus Client::writeCgiPipe()
{
    ssize_t written = 0;

    if (this->body.empty())
        return RET_OK;
    written = write(this->cgiPipe.getInWriteFd(), &this->body[0], this->body.size());
    if (written < 0)
        return RET_ERROR;
    this->body.erase(this->body.begin(), this->body.begin() + written);
    if (this->body.empty())
        return RET_OK;
    return RET_RE;
}

RetStatus Client::readCgiPipe()
{
    char received[4096];
    ssize_t length = read(this->cgiPipe.getOutReadFd(), received, 4095);
    received[length] = '\0';
    this->response.append(received, length);
    if (length < 0)
        return RET_ERROR;
    if (length == 0)
    {
        RetStatus ret = this->checkCgiExited();
        if (ret == RET_OK)
        {
            this->cgiResponse = cgiParser.parseCgiOutput(this->response);
            return ret;
        }
        return ret;
    }
    return RET_RE;
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
        return RET_RE;
    if (result < 0)
        return RET_ERROR;
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == 0)
            return RET_OK;
        std::cout << "cgi exited with code " << WEXITSTATUS(status) << std::endl;
        return RET_ERROR;
    }
    if (WIFSIGNALED(status))
    {
        std::cout << "cgi killed by signal " << WTERMSIG(status) << std::endl;
        return RET_ERROR;
    }
    return RET_ERROR;
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

bool Client::getRunCgi() { return this->runCgi; }

pid_t Client::getPid() { return this->pid; }

CharDq &Client::getCharDq(void) { return this->recDq; }

Socket &Client::getSocket() { return *this->clientSocket; }

int Client::getStatusCode() { return this->statusCode; }

Request Client::getRequest() {return this->request; }

void Client::setRunCgi(bool value) { this->runCgi = value; }

void Client::setStatusCode(int statusCode) { this->statusCode = statusCode; }

void Client::setPid(pid_t pid) { this->pid = pid; }

void Client::setListenFd(int fd) { this->listenFd = fd; }

int Client::getListenFd(void) const { return this->listenFd; }

bool Client::checkAlive(void) { return this->getSocket().checkTimeOut(); }

void Client::timeSet(time_t addTime) { this->clientSocket->setTimeStatus(addTime); }

bool Client::checkRunCgi(LocationConfig config) 
{
    if (this->runCgi)
        return false;
    if (config.getCgiExtension() == "")
        return false;
    if (access(config.getCgiPath().c_str(), X_OK))
        return false;
    if (access(config.getRoot().c_str(), X_OK))
        return false;
    return true; 
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

void Client::resetForNextRequest()
{
    this->request = Request();
    this->statusCode = 0;
    this->response.clear();
}
