#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "main.hpp" // 차후에 main.hpp를 제거하고 필요한 client에 관련된 헤더파일은 hpp에서 직접 include 하도록 수정
#include "Socket.hpp"

/**
 * @brief getPipeFd 함수에서 이 플레그를 전달해 CGI에서 http의 요청에서 body을 전달하는 inPipe의 쓰기 끝을 반환하는 flag
 */
#define InFlag 0
/**
 * @brief getPipeFd 함수에서 이 플레그를 전달해 CGI에서 생성된 http요청을 받는 OutPipe의 읽기 끝을 반환하는 flag
 */
#define OutFlag 1

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
         * @var recVec
         * @brief recv로 수신받은 HTTP 요청 원본 데이터를 누적해서 담고 있는 deque<char> 버퍼
         * 
         * 논블로킹 I/O 특성상 한 번에 전체 요청이 오지 않을 수 있으므로, 데이터가 완전히 수신될 때까지 이곳에 복사합니다.
         */
        CharQue recQue;
        /**
         * @var env
         * @brief 프로세스 기본 환경변수에 HTTP 요청(헤더, 쿼리 스트링 등)에서 추출한 CGI Meta-variable을 병합한 map 컨테이너
         * 
         * CGI 스크립트를 자식 프로세스로 실행할 때, execve의 인자로 넘겨주기 위한 환경변수를 구성하기 위해 사용됩니다.
         */
        EnvMap env;
        /**
         * @var inPipe
         * @brief Server -> CGI 프로그램 방향으로 데이터를 전달하기 위한 pipe FD 배열 (크기 2)
         */
        int inPipe[2];
        /**
         * @var outPipe
         * @brief CGI 프로그램 -> Server 방향으로 실행 결과를 받아오기 위한 pipe FD 배열 (크기 2)
         */
        int outPipe[2];
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
         * @brief CGI 실행 시 할당된 자식 프로세스의 PID를 설정하는 함수
         * 
         * fork()를 통해 CGI를 실행한 후, 부모 프로세스(Server)에서 반환받은 자식의 PID를 저장합니다.
         * @param pid 자식 프로세스 PID
         */
        void setPid(pid_t pid);

        /**
         * @brief CGI 통신을 위한 파이프 FD를 설정하는 함수
         * 
         * CGI통신을 위한 파이프세팅, 자원회수를 위해 pipe() 시스템 콜로 생성된 두 개의 파이프를 Client 객체에 저장하는 함수입니다.
         * @param inPipe 입력용 파이프 배열
         * @param outPipe 출력용 파이프 배열
         */
        void setPipeFd(int inPipe[2], int outPipe[2]);

        /**
         * @brief 클라이언트의 statuscode를 설정하는 함수
         * 
         * 요청 파싱 결과에 따라 200 OK, 400 Bad Request 등 클라이언트의 현재 요청 상태를 기록합니다.
         */
        void setStatusCode(int statusCode);

        /**
         * @brief recv로 수신된 데이터를 버퍼(recQue<char> 디큐)에 추가하는 함수
         * 
         * 논블로킹 소켓에서 조각나서 도착하는 데이터를 recVec 뒤에 계속 덧붙여 온전한 HTTP 메세지를 구성하기 위해 사용합니다.
         * @param length 수신된 데이터의 길이
         * @param received 수신된 데이터 배열
         */
        void charQueAppend(int length, char *received);

        /**
         * @brief 클라이언트의 수신 버퍼를 반환하는 함수
         * 
         * 버퍼에 담긴 데이터를 파싱 엔진에 넘겨주거나, HTTP 본문을 분석할 때 외부에서 접근하기 위해 사용합니다.
         * @return CharQue 참조
         */
        CharQue &getCharQue(void);

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

        /**
         * @brief pipe를 close하는 함수
         * @param pipe pipeFD를 담고 있는 size 2인 int 배열
         * @details CGI 실행이 종료되거나 에러가 발생하여 더 이상 필요 없는 파이프의 읽기/쓰기 끝단을 모두 닫을 때 사용합니다.
         */
        void pipeClose(int *pipe);
};


#endif