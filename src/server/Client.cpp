#include "Client.hpp"
#include "HttpUtils.hpp"
#include <sstream>
#include <cerrno>

Client::Client()
{
    this->clientSocket = 0;
    this->runCgi = false;
    this->pid = -1;
    this->shouldClose = false;
    this->listenFd = -1;
    this->sentOffset = 0;
}

Client::Client(Socket *socket, EnvMap env)
{
    this->clientSocket = socket;
    this->env = env;
    this->runCgi = false;
    this->pid = -1;
    this->shouldClose = false;
    this->listenFd = -1;
    this->sentOffset = 0;
}

Client::~Client()
{
    delete this->clientSocket;
    // cgiPipe 소멸자가 열려있는 fd를 자동으로 닫음
}

RetStatus Client::writeCgiPipe()
{
    ssize_t written = 0;

    if (this->request.body.empty())
        return RET_OK;
    written = write(this->cgiPipe.getInWriteFd(), &this->request.body[0], this->request.body.size());
    if (written < 0)
        return RET_ERROR;
    this->request.body.erase(this->request.body.begin(), this->request.body.begin() + written);
    if (this->request.body.empty())
        return RET_OK;
    return RET_RE;
}

RetStatus Client::readCgiPipe()
{
    char received[4096];
    ssize_t length = read(this->cgiPipe.getOutReadFd(), received, 4095);
    if (length < 0)
        return RET_ERROR;
    received[length] = '\0';
    this->cgiRawOutput.append(received, length);
    if (this->cgiRawOutput.size() > MAX_CGI_OUTPUT_LENGTH)
        return RET_ERROR;
    if (length == 0)
    {
        RetStatus ret = this->checkCgiExited();
        if (ret == RET_OK)
            this->cgiResponse = cgiParser.parseCgiOutput(this->cgiRawOutput);
        return ret;
    }
    return RET_RE;
}

void Client::clearCgiRawOutput(void)
{
    this->cgiRawOutput.clear();
}

const Response &Client::getCgiResponse() const
{
    return this->cgiResponse;
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
        return RET_ERROR;
    }
    if (WIFSIGNALED(status))
    {
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

Request Client::getRequest() {return this->request; }

void Client::setRunCgi(bool value) { this->runCgi = value; }

void Client::fail(Status status, FailMode mode)
{
    this->routeResult.action = ACTION_ERROR;
    this->routeResult.errorCode = status;
    this->routeResult.allowedMethods.clear();
    if (mode == FAIL_CLOSE)
        this->shouldClose = true;
}

void Client::setPid(pid_t pid) { this->pid = pid; }

void Client::setListenFd(int fd) { this->listenFd = fd; }

int Client::getListenFd(void) const { return this->listenFd; }

bool Client::checkAlive(void) { return this->getSocket().checkTimeOut(); }

void Client::timeSet(time_t addTime) { this->clientSocket->setTimeStatus(addTime); }

bool Client::checkRunCgi(const LocationConfig &config, const std::string &resolvedPath, int &errorCode)
{
    if (access(config.getCgiPath().c_str(), X_OK))
    {
        errorCode = STATUS_INTERNAL_SERVER_ERROR;
        return false;
    }
    const std::string &base = config.getAlias().empty() ? config.getRoot() : config.getAlias();
    if (access(base.c_str(), X_OK))
    {
        errorCode = STATUS_INTERNAL_SERVER_ERROR;
        return false;
    }
    if (access(resolvedPath.c_str(), R_OK))
    {
        errorCode = (errno == EACCES) ? STATUS_FORBIDDEN : STATUS_NOT_FOUND;
        return false;
    }
    return true;
}

ReqParseResult Client::onReceive()
{
    parser.parse(recDq);
    ReqParseResult ret = parser.getState();
    if (ret == REQ_PARSE_ERROR) shouldClose = true;
    if (ret == REQ_PARSE_INCOMPLETE) return ret;
    request = parser.getRequest();
    if (ret == REQ_PARSE_DONE)
    {
        std::map<std::string, std::string>::const_iterator it = request.headers.find("connection");
        shouldClose = false;
        if (it != request.headers.end())
        {
            std::stringstream ss(it->second);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                size_t s = token.find_first_not_of(" \t");
                size_t e = token.find_last_not_of(" \t");
                if (s == std::string::npos) continue;
                if (HttpUtils::toLower(token.substr(s, e - s + 1)) == "close")
                {
                    shouldClose = true;
                    break;
                }
            }
        }
    }
    parser.clear();
    return ret;
}

bool Client::getShouldClose() const
{
    return shouldClose;
}

bool Client::hasIncompleteRequest() const
{
    return !(parser.isIdle() && recDq.empty());
}

void Client::resetForNextRequest()
{
    this->request = Request();
    this->response.clear();
    this->sentOffset = 0;
    this->cgiRawOutput.clear();
    this->routeResult = RouteResult();
}

size_t Client::getSentOffset() const
{
    return this->sentOffset;
}

void Client::addSentOffset(size_t length)
{
    this->sentOffset += length;
}

void Client::resetSentOffset()
{
    this->sentOffset = 0;
}

void Client::setRouteResult(const RouteResult &result) { this->routeResult = result; }

const RouteResult &Client::getRouteResult() const { return this->routeResult; }

void Client::setMaxBodyLength(size_t length)
{
    this->parser.setMaxBodyLength(length);
}
