#include "Client.hpp"
#include "HttpUtils.hpp"
#include <sstream>

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

int Client::writeCgiPipe()
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

int Client::readCgiPipe()
{
    char received[4096];
    ssize_t length = read(this->cgiPipe.getOutReadFd(), received, 4095);
    if (length < 0)
        return RET_ERROR;
    if (length == 0)
        return this->checkCgiExited();
    received[length] = '\0';
    this->response.append(received, length);
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

pid_t Client::getPid() { return this->pid; }

CharDq &Client::getCharDq(void) { return this->recDq; }

Socket &Client::getSocket() { return *this->clientSocket; }

int Client::getStatusCode() { return this->statusCode; }

Request Client::getRequest() {return this->request; }

bool Client::checkRunCgi(void) { return this->runCgi; }

void Client::setRunCgi(bool value) { this->runCgi = value; }

void Client::setStatusCode(int statusCode) { this->statusCode = statusCode; }

void Client::setPid(pid_t pid) { this->pid = pid; }

void Client::setListenFd(int fd) { this->listenFd = fd; }

int Client::getListenFd(void) const { return this->listenFd; }

bool Client::checkAlive(void) { return this->getSocket().checkTimeOut(); }

void Client::timeSet(time_t addTime) { this->clientSocket->setTimeStatus(addTime); }

ReqParseResult Client::onReceive()
{
    parser.parse(recDq);
    ReqParseResult ret = parser.getState();
    if (ret == REQ_PARSE_ERROR) shouldClose = true;
    if (ret == REQ_PARSE_INCOMPLETE) return ret;
    request = parser.getRequest();
    if (ret == REQ_PARSE_DONE)
    {
        // HTTP/1.1 기본값은 keep-alive이므로, Connection 헤더가 없거나
        // "close" 토큰이 없으면 shouldClose는 false로 유지된다.
        // 헤더 key는 RequestParser::transferHeaders()에서 이미 소문자로
        // 정규화되어 저장되므로 "connection"으로 조회하면 되지만,
        // value는 그대로 보존되므로 비교 시 대소문자를 무시해야 한다.
        // Connection 헤더 값은 RFC 7230 §6.1에 따라 콤마로 구분된 토큰
        // 리스트일 수 있으므로(예: "keep-alive, close"), 값 전체를
        // "close"와 완전일치 비교하지 않고 RequestParser::validateTransferEncoding()과
        // 동일한 방식으로 토큰 단위로 분리해 trim + 소문자 비교한다.
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
    this->routeResult = RouteResult();
}

void Client::setRouteResult(const RouteResult &result) { this->routeResult = result; }

const RouteResult &Client::getRouteResult() const { return this->routeResult; }

void Client::setMaxBodyLength(size_t length)
{
    this->parser.setMaxBodyLength(length);
}
