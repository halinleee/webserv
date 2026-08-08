#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "Pipe.hpp"
#include "Socket.hpp"
#include "RequestParser.hpp"
#include "RouteResult.hpp"
#include "ServerConfig.hpp"
#include "CgiParser.hpp"
#include "Response.hpp"
#include "type.hpp"

#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>

/**
 * @brief getPipeFd 함수에서 이 플레그를 전달해 CGI에서 http의 요청에서 body을 요청할때 사용하는 flag
 */
enum PipeFlag
{
    InFlag = 0,
    OutFlag = 1
};

/**
 * @brief 서버에 연결된 단일 클라이언트의 정보와 상태를 관리하는 클래스
 * 
 * 클라이언트별 소켓 정보, 수신된 데이터 버퍼, CGI 환경변수, 파이프 FD 및 자식 프로세스 PID 등을
 * 종합적으로 캡슐화하여 관리합니다. Server 클래스 내에서 클라이언트 연결이 유지되는 동안의 모든 상태(Context)를 보존하며,
 * HTTP 요청 파싱과 응답 생성, CGI 실행 등의 작업 시 이 클래스의 데이터를 활용 후 response를 보내고 자원을 회수합니다.
 */
class Client
{
    private:
        /**
         * @var clientSocket
         * @brief 클라이언트의 소켓 FD, addr(port, ip주소등 관련 정보), time(마지막 요청 시간)을 관리하는 Socket 객체 포인터
         * 
         * accept() 성공 시 동적 할당되며, 클라이언트의 port, ip주소등 클라이언테에 대한 정보를 가지고 있습니다.
         */
        Socket *clientSocket;

        /**
         * @var runCgi
         * @brief Cgi의 동작여부를 담고있는 bool 변수
         * 
         * TimeOut, Cgi분기s에서 사용
         */
        bool runCgi;
        /**
         * @var recVec
         * @brief recv로 수신받은 HTTP 요청 원본 데이터를 누적해서 담고 있는 deque<char> 버퍼
         * 
         * 논블로킹 I/O 특성상 한 번에 전체 요청이 오지 않을 수 있으므로, 데이터가 완전히 수신될 때까지 이곳에 복사합니다.
         */
        CharDq recDq;
        /**
         * @var env
         * @brief 프로세스 기본 환경변수에 HTTP 요청(헤더, 쿼리 스트링 등)에서 추출한 CGI Meta-variable을 병합한 map 컨테이너
         * 
         * CGI 스크립트를 자식 프로세스로 실행할 때, execve의 인자로 넘겨주기 위한 환경변수를 구성하기 위해 사용됩니다.
         */
        EnvMap env;
        /**
         * @var cgiPipe
         * @brief CGI 통신용 파이프 쌍. Client 소멸 시 ~Pipe()가 자동으로 fd를 정리합니다.
         */
        Pipe cgiPipe;
        /**
         * @var statusCode
         * @brief HTTP 응답을 생성할 때 기준이 되는 상태 코드 (ex: 200, 404, 500)
         * 
         * 파싱 중 에러 발생, 파일 없음, 권한 부족 등의 예외 상황 시 적절한 에러 페이지를 응답하기 위해 상태를 기록합니다.
         */
        int statusCode;
        /**
         * @var pid
         * @brief 실행된 CGI 자식 프로세스의 PID
         * 
         * CGI 실행 완료 후 waitpid()를 통해 좀비 프로세스 방지 및 정상 종료 확인, 타임아웃 시 강제 종료(kill) 목적으로 사용합니다.
         */
        pid_t pid;

        /**
         * @var cgiRawOutput
         * @brief CGI 파이프에서 읽은 stdout 원본 바이트를 파싱 전까지 누적하는 임시 버퍼
         *
         * readCgiPipe()가 EOF까지 누적한 뒤 CgiParser로 파싱해 cgiResponse에 저장하고 나면
         * 더 이상 필요 없는 값입니다. response(최종 송신 버퍼)와는 별개입니다.
         */
        std::string cgiRawOutput;

        /**
         * @var cgiResponse
         * @brief CGI stdout을 파싱한 결과(상태 코드, 헤더, 바디)를 담는 구조체
        */
        Response cgiResponse;

        RequestParser parser;
        Request request;
        bool shouldClose;

        /**
         * @var listenFd
         * @brief 이 클라이언트가 accept된 리스닝 소켓(서버 포트)의 FD
         *
         * 클라이언트가 어느 server 블록(포트)으로 접속했는지 추적해, CGI 등에서 해당 포트의 ServerConfig를 찾아 쓰기 위해 사용됩니다.
         */
        FD listenFd;
        CgiParser cgiParser;

        /**
         * @var routeResult
         * @brief clientRequest 단계에서 Router::route()로 계산된 라우팅 결과
         *
         * clientRequest(EPOLLIN)와 clientResponse(EPOLLOUT)가 서로 다른 epoll 이벤트 턴에서
         * 호출되므로, 요청 단계에서 계산한 라우팅 결과를 응답 단계까지 들고 있기 위해 사용합니다.
         */
        RouteResult routeResult;

        /**
         * @var sentOffset
         * @brief response 버퍼 중 이미 send()로 전송 완료한 바이트 수
         *
         * 논블로킹 소켓에서 send()가 response 전체를 한 번에 다 보내지 못할 때,
         * 남은 부분을 다시 복사해서 잘라내는 대신 이 오프셋만 전진시켜 다음 EPOLLOUT에서
         * 이어 보낼 위치를 추적하기 위해 사용합니다.
         */
        size_t sentOffset;

    public:
        /**
         * @brief Client의 기본 생성자
         * 
         * 초기화되지 않은 Client 객체 생성을 방지하기 위해 선언되었지만 구현/사용되지 않습니다.
         */
        Client();

        /**
         * @brief Client 객체의 private변수를 초기화하는 생성자
         * 
         * Server가 accept()로 새 클라이언트를 받아들일 때 호출되어 클라이언트 컨텍스트를 초기화합니다.
         * @param socket 클라이언트와 연결된 Socket 객체의 포인터 관련 정보가 담겨있는 addr과 마지막 사용시간인 time도 가지고 있음
         * @param env 클라이언트에 할당할 환경변수 맵(서버에서 복사한 기본 환경변수)
         */
        Client(Socket *socket, EnvMap env);

        /**
         * @brief Client의 소멸자
         * 
         * 현재 socket에 대해서 delete를 추가함
         * @todo 나중에 pipeClose관련 로직 추가
         */
        ~Client();

        /**
         * @var response
         * @brief 클라이언트에게 전송할 최종 HTTP 응답 메세지를 담는 문자열
         *
         * STATIC/REDIRECT/ERROR/CGI 처리 결과를 Response::toString()으로 직렬화한 값이 저장되며,
         * serverSend()가 이 버퍼를 그대로 소켓에 씁니다.
        */
        std::string response;

        /** 
         * @brief cgi(child 프로세스)가 정상 종료가 되었는지 확인하는 함수
         * 
        */
        RetStatus checkCgiExited(void);
        
        /**
         * @brief 이 클라이언트의 keep-alive가 유지를 하는지 확인하는 함수
        */
        bool checkAlive(void);

        /**
         * @brief 이 클라이언트의 cgi가 실행이 가능한지 확인하는 함수
        */
        bool checkRunCgi(const LocationConfig &config, const std::string &resolvedPath, bool &notFound);

        bool getRunCgi();

        /**
         * @brief 클라이언트의 keepAlive시간을 초기화하는 함수
        */
        void timeSet(time_t addTime);

        /**
         * @brief Cgi프로그램에게 넘길 body내용을 Cgi프로그램과 연결되어 있는 파이프에 적는 함수
         * 
         * @return 0(RET_ERROR) 에러 발생
         * @return 1(RET_OK) 정상 동작
         * @return 1(RET_RE) 정상 동작은 했으나 pipe의 크기 제한으로 다시 이 함수를 와야할 경우
        */
        RetStatus writeCgiPipe(void);

        /**
         * @brief Cgi프로그램이 보낸 결과를 파이프에서 읽어오는 함수
         *
         * EOF(pipe write end가 모두 닫힘)을 감지하면 checkCgiExited()를 호출해
         * 자식 프로세스가 실제로 종료됐는지 확인하고 waitpid로 회수(좀비 방지)한다.
         *
         * @return 0(RET_ERROR) 읽기 에러 또는 CGI가 비정상 종료(exit code != 0, signal)
         * @return 1(RET_OK) EOF + CGI 정상 종료 확인 완료(모든 데이터를 다 읽음)
         * @return 2(RET_RE) 아직 읽을 데이터가 남아있음, 또는 EOF 후 자식이 아직 reap되지 않아 재시도 필요
         */
        RetStatus readCgiPipe(void);

        /**
         * @brief CGI 파이프 읽기 실패/타임아웃 시 누적 중이던 raw stdout 버퍼를 비우는 함수
         */
        void clearCgiRawOutput(void);

        void setRunCgi(bool value);
        /**
         * @brief CGI 실행 시 할당된 자식 프로세스의 PID를 설정하는 함수
         * 
         * fork()를 통해 CGI를 실행한 후, 부모 프로세스(Server)에서 반환받은 자식의 PID를 저장합니다.
         * @param pid 자식 프로세스 PID
         */
        void setPid(pid_t pid);

        /**
         * @brief 클라이언트가 accept된 리스닝 소켓의 FD를 설정하는 함수
         * @param fd 리스닝 소켓의 FD
         */
        void setListenFd(int fd);

        /**
         * @brief 클라이언트가 accept된 리스닝 소켓의 FD를 반환하는 함수
         * @return 리스닝 소켓의 FD
         */
        int getListenFd(void) const;

        /**
         * @brief CGI 파이프 객체의 참조를 반환합니다.
         *
         * Server::cgiRun에서 init(), excute 인자 전달, closeChildSide() 등을 직접 호출하기 위해 사용합니다.
         */
        Pipe &getCgiPipe();

        /**
         * @brief 클라이언트의 statuscode를 설정하는 함수
         * 
         * 요청 파싱 결과에 따라 200 OK, 400 Bad Request 등 클라이언트의 현재 요청 상태를 기록합니다.
         */
        void setStatusCode(int statusCode);

        /**
         * @brief 요청 파싱이 끝나기 전에 서버가 강제로 요청의 상태를 확정할 때 쓰는 함수
         *
         * readTimeout 등으로 요청을 끝까지 받지 못한 채 응답을 보내야 할 때 사용합니다.
         * clientResponse가 STATUS_UNDEFINED 여부로 라우팅 필요 유무를 판단하므로, 이 함수로
         * request.status를 채워 라우팅 없이 바로 에러 응답이 만들어지도록 하고, 이후 keep-alive를
         * 이어가지 않도록 shouldClose도 함께 true로 설정합니다.
         * @param status 확정할 상태 코드 (ex: STATUS_REQUEST_TIMEOUT)
         */
        void setRequestStatus(int status);

        /**
         * @brief recv로 수신된 데이터를 버퍼(recDq<char> 디큐)에 추가하는 함수
         * 
         * 논블로킹 소켓에서 조각나서 도착하는 데이터를 recVec 뒤에 계속 덧붙여 온전한 HTTP 메세지를 구성하기 위해 사용합니다.
         * @param length 수신된 데이터의 길이
         * @param received 수신된 데이터 배열
         */
        void CharDqAppend(int length, unsigned char *received);

        /**
         * @brief 클라이언트의 수신 버퍼를 반환하는 함수
         * 
         * 버퍼에 담긴 데이터를 파싱 엔진에 넘겨주거나, HTTP 본문을 분석할 때 외부에서 접근하기 위해 사용합니다.
         * @return CharDq 참조
         */
        CharDq &getCharDq(void);

        /**
         * @brief 클라이언트의 소켓 객체를 반환하는 함수
         * 
         * 클라이언트의 FD를 구해 epoll에 이벤트를 변경하거나(epCtl), 클라이언트 정보를 조회할 때 사용합니다.
         * @return Socket 객체 참조
         */
        Socket &getSocket();

        /**
         * @brief CGI 자식 프로세스의 PID를 반환하는 함수
         * @return 프로세스의 PID
         * @details 클라이언트 연결 해제 시 CGI가 여전히 실행 중인지 확인하고 자원을 회수할 때 사용합니다.
         */
        pid_t getPid(void);

        /**
         * @brief 클라이언트의 statuscode를 반환
         * @return 클라이언트의 statusCode
         * @details Response 생성 단계에서 상태 코드를 확인하여 적절한 HTTP 헤더와 본문을 구성할 때 사용합니다.
         */
        int getStatusCode();

        /**
         * @brief Pipe의 FD를 반환하는 함수
         * @param index Inflag = inPipe의 쓰기끝, OutFlag = outPipe의 읽기끝
         * @return 파이프의 FD
         * @details Server에서 epoll 이벤트를 통해 파이프 I/O가 가능한지 확인하고 read/write를 수행할 때 해당 FD를 얻기 위해 사용됩니다.
         */
        int getPipeFd(int index);

        Request getRequest();

        /**
         * @brief InFlag 또는 OutFlag에 해당하는 파이프 끝단을 닫는 함수
         *
         * @param flag InFlag = inWriteFd 닫기, OutFlag = outReadFd 닫기
         */
        void pipeClose(int flag);
        ReqParseResult onReceive();
        bool getShouldClose() const;

        /**
         * @brief 이 클라이언트가 속한 리스닝 소켓의 ServerConfig에서 조회한 client_max_body_size를
         * 요청 파서에 반영하는 함수
         * @param length 허용할 최대 body 길이(바이트)
         */
        void setMaxBodyLength(size_t length);

        /**
         * @brief clientRequest 단계에서 계산된 라우팅 결과(RouteResult)를 저장하는 함수
         * @param result Router::route()가 반환한 라우팅 결과
         */
        void setRouteResult(const RouteResult &result);

        /**
         * @brief clientRequest 단계에서 저장해둔 라우팅 결과(RouteResult)를 반환하는 함수
         * @return clientResponse에서 분기 처리에 사용할 RouteResult 참조
         */
        const RouteResult &getRouteResult() const;

        /**
         * @brief keep-alive 연결에서 응답 송신 완료 후 다음 요청을 받기 위해 상태를 초기화하는 함수
         *
         * recDq(수신 버퍼)는 파이프라이닝된 다음 요청의 데이터가 남아있을 수 있으므로 비우지 않고,
         * 직전 요청에 대한 정보인 request와 statusCode만 초기화합니다. (parser는 onReceive에서 이미 clear됨)
         * routeResult도 이전 요청의 라우팅 결과가 다음 요청에 잘못 사용되지 않도록 함께 초기화합니다.
         */
        void resetForNextRequest();

        /**
         * @brief readCgiPipe()가 파싱해둔 CGI 응답(상태 코드, 헤더, 바디)을 반환하는 함수
         * @return clientResponse에서 ACTION_CGI 분기 처리에 사용할 Response 참조
         */
        const Response &getCgiResponse() const;

        /**
         * @brief response 버퍼 중 이미 전송 완료한 바이트 수(sentOffset)를 반환하는 함수
         */
        size_t getSentOffset() const;

        /**
         * @brief sentOffset을 length만큼 전진시키는 함수
         *
         * serverSend()에서 send()가 일부만 전송했을 때, 다음 EPOLLOUT에서 이어 보낼
         * 위치를 갱신하기 위해 사용합니다.
         */
        void addSentOffset(size_t length);

        /**
         * @brief sentOffset을 0으로 초기화하는 함수
         *
         * 새 response를 빌드해서 대입할 때(clientResponse, clientAccept) 및
         * resetForNextRequest()에서 호출합니다.
         */
        void resetSentOffset();
};


#endif