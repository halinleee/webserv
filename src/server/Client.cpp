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

/**
 * @brief Cgi프로그램에게 넘길 body내용을 Cgi프로그램과 연결되어 있는 파이프에 적는 함수
 */
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

/**
 * @brief Cgi프로그램이 보낸 결과를 파이프에서 읽어오는 함수
 */
int Client::readCgiPipe()
{
    int status;
    char received[4096];
    ssize_t length = 0;

    length = read(this->outPipe[0], received, 4095);
    received[length] = '\0';
    this->response.append(received, length);
    length = read(this->outPipe[0], received, 4095);
    length = 0;
    if (length <= 0)
    {
        if (length < 0)
            return (STATUS_ERROR);
        int result = waitpid(this->pid, &status, WNOHANG);
        if (result < 0)
            return (STATUS_ERROR);
        else
            return (STATUS_OK);
    }
    std::cout << "not meet eof" << std::endl;
    return (STATUS_RE);
}

/**
 * @brief 이 클라이언트의 keep-alive가 유지를 하는지 확인하는 함수
 */
bool Client::checkAlive(void)
{
    if (std::time(NULL) >= this->getSocket().getTimeOut())
        return (false);
    else
        return (true);
}

/**
 * @brief 클라이언트의 keepAlive시간을 초기화하는 함수
 */
void Client::timeSet(void)
{
    this->getSocket().actTimeSet();
}

/**
 * @brief 소켓을 통해 받은 http요청을 http 파서에게 넘겨주기 위해서 Clinet의 CharDq에 담는 함수
 * 
 * @param length 받은 메세지의 길이(덱에 넣기 위해서 length를 알아야하기 떄문에 사용)
 * @param received 받은 메세지의 내용
 */
void Client::CharDqAppend(int length, unsigned char *received)
{
    this->recDq.insert(this->recDq.end(), received, received + length);
}

/**
* @brief 클라이언트의 수신 버퍼를 반환하는 함수
* 
* 버퍼에 담긴 데이터를 파싱 엔진에 넘겨주거나, HTTP 본문을 분석할 때 외부에서 접근하기 위해 사용합니다.
* @return CharDq 참조
*/
CharDq &Client::getCharDq(void)
{
    return (this->recDq);
}

/**
* @brief 클라이언트의 소켓 객체를 반환하는 함수
* 
* 클라이언트의 FD를 구해 epoll에 이벤트를 변경하거나(epCtl), 클라이언트 정보를 조회할 때 사용합니다.
* @return Socket 객체 참조
*/
Socket &Client::getSocket()
{
    return (*this->clientSocket);
}

/**
* @brief 클라이언트의 statuscode를 반환
* @return 클라이언트의 statusCode
* @details Response 생성 단계에서 상태 코드를 확인하여 적절한 HTTP 헤더와 본문을 구성할 때 사용합니다.
*/
int Client::getStatusCode()
{
    return (this->statusCode);
}

/**
* @brief 클라이언트의 statuscode를 설정하는 함수
* 
* 요청 파싱 결과에 따라 200 OK, 400 Bad Request 등 클라이언트의 현재 요청 상태를 기록합니다.
*/
void Client::setStatusCode(int statusCode)
{
    this->statusCode = statusCode;
}

/**
 * @brief pipe를 close하는 함수
 * 
 * CGI 실행이 종료되거나 에러가 발생하여 더 이상 필요 없는 파이프의 읽기/쓰기 끝단을 모두 닫을 때 사용합니다.
 * @param pipe pipeFD를 담고 있는 size 2인 int 배열
 */
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

/**
 * @brief pipe를 close하는 함수
 * 
 * CGI 실행이 종료되거나 에러가 발생하여 더 이상 필요 없는 파이프의 읽기/쓰기 끝단을 모두 닫을 때 사용합니다.
 * @param pipe pipeFD를 담고 있는 size 2인 int 배열
 */
void Client::pipeClose(int pipe[2])
{
    if (pipe[0] > -1)
        close(pipe[0]);
    if (pipe[1] > -1)
        close(pipe[1]);
    pipe[0] = -1;
    pipe[1] = -1;
}

/**
 * @brief CGI통신을 위해 생성된 pipe를 두 개의 파이프를 Client 객체에 저장하는 함수
 * 
 * 추가적으로 Nonblocking 세팅, 불 필요한 FD에 대해서 close를 수행함
 * @param inPipe 입력용 파이프 배열
 * @param outPipe 출력용 파이프 배열
*/
void Client::setPipeFd(int inPipe[2], int outPipe[2])
{
    close(inPipe[0]);
    close(outPipe[1]);
    this->inPipe[0] = -1;
    this->inPipe[1] = inPipe[1];
    this->outPipe[0] = outPipe[0];
    this->outPipe[1] = -1;
}

/**
 * @brief Pipe의 FD를 반환하는 함수
 * 
 * Server에서 epoll 이벤트를 통해 파이프 I/O가 가능한지 확인하고 read/write를 수행할 때 해당 FD를 얻기 위해 사용됩니다.
 * @param index Inflag = inPipe의 쓰기끝, OutFlag = outPipe의 읽기끝
 * @return 파이프의 FD
*/
int Client::getPipeFd(int index)
{
    if (index == InFlag)
        return (this->inPipe[1]);
    else
        return (this->outPipe[0]);
}

/**
 * @brief CGI 실행 시 할당된 자식 프로세스의 PID를 설정하는 함수
 * 
 * fork()를 통해 CGI를 실행한 후, 부모 프로세스(Server)에서 반환받은 자식의 PID를 저장합니다.
 * @param pid 자식 프로세스 PID
*/
void Client::setPid(pid_t pid)
{
    this->pid = pid;
}

/**
 * @brief CGI 자식 프로세스의 PID를 반환하는 함수
 * 
 * 클라이언트 연결 해제 시 CGI가 여전히 실행 중인지 확인하고 자원을 회수할 때 사용합니다.
 * @return 프로세스의 PID
 */
pid_t Client::getPid()
{
    return (this->pid);
}

Client::~Client()
{
    this->pipeClose(this->inPipe);
    this->pipeClose(this->outPipe);
    delete this->clientSocket;
}