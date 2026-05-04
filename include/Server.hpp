#ifndef SERVER_HPP
# define SERVER_HPP

#include "main.hpp"
#include "Epoll.hpp"
#include "Utils.hpp"
#include "Socket.hpp"
#include "Client.hpp"
#include "Cgi.hpp"

#include <sys/wait.h>

/**
 * @brief 웹 서버의 전반적인 동작과 클라이언트 연결, 이벤트를 관리하는 핵심 클래스
 * 
 * 싱글 스레드 기반의 논블로킹 I/O(epoll)를 사용하여 다수의 클라이언트 요청을 동시에 처리합니다.
 * 서버 초기화, 포트 바인딩, 이벤트 루프 실행, HTTP 요청/응답 관리 및 CGI 통신 등 서버의 전체 비즈니스 흐름을 통제합니다.
 */
class Server
{
    private:
        /**
         * @var serverActive
         * @brief server의 메인 loop에서 사용되며 signal을 받아서 서버를 종료하기 위해 사용
         */
        bool serverActive;
        /**
         * @var serverSocket 
         * @brief 서버가 연결을 대기하는 리스닝 소켓을 관리하는 객체 포인터
         * 
         * bind() 및 listen()이 완료된 서버 소켓으로, epoll에서 EPOLLIN 이벤트가 발생하면 accept()를 호출하는 대상이 됩니다.
         * 안에 서버의 소켓FD, addr, time등의 정보를 socket구조체로 가지고 있습니다.
         */
        Socket *serverSocket;
        /**
         * @var client 
         * @brief 현재 서버에 접속 중인 모든 클라이언트의 FD를 인덱스로 클라이언트 객체의 포인터를 가지고 있는 vector
         * 
         * epoll 이벤트가 발생했을 때 발생한 FD가 어떤 클라이언트의 것인지 식별하고, 해당 클라이언트의 수신 버퍼나 상태를 즉각적으로 조회/수정하기 위해 사용됩니다.
        */
        ClientVec client;

        /**
         * @var inClientVec
         * @brief 현재 서버에 어떤 클라이언트의 FD가 있는지에 대한 정보를 가지고 있는 Vector
         * 
         * keep-alive를 구현하기 위해서 사용(pipeFd와 clinet socketFD가 섞이기 떄문에 해결하기 위해서 사용)
         */
        FdVec inClientVec;
        /**
         * @var pipeToClientMap
         * @brief 파이프의 FD와 클라이언트의 FD를 매핑해서 가지고 있는 <int, int>맵
         * 
         * Epoll에 이밴트를 감지하기 위해 Pipe FD을 등록함 때문에 Epoll이 CGI 통신용 파이프의 FD에 대한 이벤트를 반환했을 때, 이 파이프가 어떤 클라이언트의 요청을 처리 중인지
         * 빠르게 역추적하기 위해 사용됩니다.
        */
        IntMap pipeToClientMap;
        /**
         * @var env 
         * @brief 서버 구동 시 전달받은 프로세스의 기본 OS 환경변수를 파싱해 담아둔 원본 맵
         * 
         * CGI 스크립트 실행 시 각 클라이언트마다 이 원본 환경변수 맵을 복사한 후, 클라이언트 고유의 Meta-variables를
         * 덧붙여서 자식 프로세스(execve)에 전달하는 베이스로 사용됩니다.
         */
        EnvMap env;
        
    public:
        /**
         * @brief Server 생성자. 초기 환경변수를 파싱하여 저장합니다.
         * 
         * 서버 초기화 단계에서 메모리를 확보하고 envp를 맵 형태로 변환하여 보관합니다.
         * @param envp 메인 함수에서 전달받은 환경변수
         */
        Server(char **envp);

        /**
         * @brief Client 맵과 serverSocket으로 할당받은 자원회수
         * 
         * 서버가 정상 종료되거나 예외로 인해 다운될 때, 연결된 모든 클라이언트와 서버 소켓 자원을 안전하게 해제합니다.
         */
        ~Server();

        /**
         * @brief 새로운 서버 소켓을 생성하고 Epoll에 등록하는 함수
         * 
         * 내부적으로 serverSetting()을 호출하여 소켓 옵션을 설정하고 논블로킹 모드로 만든 후, Epoll의 감시 대상에 추가합니다.
         * @param port 바인딩할 포트 번호
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @return Error 발생시 1, 정상 동작 0
         */
        bool serverAdd (in_port_t port, Epoll &epoll);

        /**
         * @brief 서버가 client에게 데이터를 송신하는 함수
         * 
         * client의 response 변수에 있는 response 내용을 보냄(error의 경우 그 상황에 맞춰서 http메세지로 바꿔서 전송하면 됨)
         * @param client 보낼 clinet 객체
         * @return 함수의 성공 여부
         */
        int serverSend(Client *client);

        /**
         * @brief Epoll 이벤트를 감지하고 종류에 따라 분기 처리하는 메인 이벤트 루프 함수
         
         * 무한 루프 내에서 epWait()을 호출하여 발생하는 이벤트(연결 요청, 데이터 수신, 송신 가능 여부 등)를
         * 판단하고 각각 clientAccept, clientRequest, clientResponse 등으로 라우팅하는 서버의 심장부 역할을 합니다.
         * @param epoll 이벤트를 관리할 Epoll 객체
         */
        bool eventProcess(Epoll &epoll);

        /**
         * @brief 서버 소켓의 바인딩 및 리슨을 설정하는 함수
         * 
         * configfile에서 파싱된 내용을 바탕으로 지정된 포트로 bind() 후 listen()을 수행합니다.
         * @param serverSocket 설정할 서버 Socket 객체
         * @return bind, listen, nonblockingSet 함수의 실패여부
         */
        bool serverSetting(Socket *serverSocket);

        /**
         * @brief 클라이언트의 연결 요청을 수락하고 Client 객체를 생성 및 Epoll에 등록하는 함수
         * 
         * 서버 소켓에 EPOLLIN 이벤트가 발생했을 때 호출되며, accept()를 통해 새 클라이언트용 FD를 받아 논블로킹으로 설정하고 Client 컨테이너에 추가합니다.
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param socket 연결 요청을 받은 서버 Socket 객체
         */
        bool clientAccept(Epoll &epoll, Socket *socket); //차후에 서버에 접속하는 클라이언트 정보를 가공할 일이 있으면 수정 필요

        /**
         * @brief eventProcess에서 client에 대한 동작(request, response, cgi)에 대한 동작을 수행하는 함수
         * @param epoll I/O 이벤트를 관리하는 epoll 객체(오류 발생 및 respose를 보낸 후에 등록했던 이벤트 삭제를 위해 매개변수로 지정)
         * @param currentFd 현재 이벤트가 감지된 FD
         * @param currentEvent 현재 이벤트의 내용
         * @return loop 동작 중 error발생 여부(발생시 STATUS_ERROR = 0, 아닐 시 STATUS_OK = 1)
         */
        bool clientLoop(Epoll &epoll, FD currentFd, u_int32_t currentEvent);

        /**
         * @brief 클라이언트의 데이터를 읽어들이고 요청을 파싱하는 함수
         * 
         * 클라이언트 소켓에 EPOLLIN 이벤트가 떴을 때 호출됩니다. recv()로 데이터를 읽어 Client의 recDq에 쌓고, 요청이 모두 도착했는지 확인합니다.
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param client 데이터를 전송한 클라이언트 객체
         */
        bool clientRequest(Epoll &epoll, Client *client); // 차후에 클라이언트 요청이 들어오는걸 파싱하는 로직이 들어가야함(현재 프린트만 하도록 동작)

        /**
         * @brief 클라이언트에게 응답 데이터를 전송하는 함수
         * 
         * 응답 메세지가 준비되었고 소켓이 쓰기 가능한 상태(EPOLLOUT)일 때 호출되어 send() 시스템 콜로 데이터를 클라이언트에 밀어넣고 통신 종료 및 자원을 회수합니다.
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param client 응답을 보낼 클라이언트 객체
         */
        bool clientResponse(Epoll &epoll, Client *client);

        /**
        * @brief 특정 FD에 해당하는 클라이언트 객체의 포인터가 존재하는지 확인하는 함수
        * @param fd 확인할 파일 디스크립터(인덱스로 사용)
        * @return 클라이언트 존재 여부(존재 시 true, 미 존재 시 false)
        */
        bool clientExist(int fd);

        /**
         * @brief CGI 프로그램을 실행하고 파이프를 설정하는 함수
         * 
         * 정적 파일이 아닌 동적 처리가 필요할 때 호출됩니다. cgi의 excute 함수를 실행하여 fork()로 자식 프로세스를 생성하고 execve()로 지정된 CGI 스크립트를 실행하며, 통신을 위한 두 쌍의 파이프를 연결 및 epoll에 등록합니다.
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param client 이벤트가 발생한 클라이언트 객체
         */
        bool cgiRun(Epoll &epoll, Client *client);

        /**
         * @brief CGI 프로세스로부터 데이터를 읽어들이는 함수
         * 
         * EPOLL에서 OutPipe에 EPOLLIN이 감지되면 실행됨
         * 
         * CGI 스크립트가 표준 출력으로 내보낸 데이터를 outPipe의 읽기 끝단에서 read()하여 응답(response) 버퍼로 구성할 때 사용됩니다.
         * @param epoll 이벤트를 관리하는 Epoll 객체
         * @param socket 이벤트가 감지된 pipe의 FD를 가지고 있는 client객체의 주소
         */
        bool cgiPipeRead(Epoll &epoll, Client *client);

        /**
         * @brief CGI 프로세스로 데이터를 전송(쓰기)하는 함수
         *
         * EPOLL에서 InPipe에 EPOLLIN이 감지되면 실행됨
         * 
         * HTTP POST 요청 등으로 들어온 Body 데이터를 inPipe의 쓰기 끝단을 통해 CGI 프로세스의 표준 입력으로 밀어넣을 때 호출됩니다.
         * @param epoll 이벤트를 관리하는 Epoll 객체
         * @param client 이벤트가 감지된 pipe의 FD를 가지고 있는 client객체의 주소
         */
        bool cgiPipeWrite(Epoll &epoll, Client *client);

        /**
         * @brief 클라이언트가 동작을 언제 했고 
         */
        bool checkKeepAlive();

        /**
         * @brief 클라이언트의 요청을 보낸 시간이 keep-alive 시간을 지났는지 확인하고 지났을 경우 해제하는 함수
         */
        void checkKeepAliveClient(int &index);
        
        
        /**
         * @brief 특정 클라이언트의 연결을 종료하고 자원을 해제하는 함수
         * 
         * 요청 처리가 완료되었거나, 타임아웃/에러 발생 시 호출되어 Client 객체를 map에서 제거하고 메모리를 해제합니다.
         * @param deleteFd 삭제할 클라이언트의 소켓 FD
         */
        void deleteClient(int deleteFd);

        /**
         * @brief 빌드된 클라이언트의 response의 내용을 반환하는 함수
         * 
         * request로 날라온 http를 해석하고 동작 후 나온 response의 주소를 반환
         */
        std::string getResponse(void);

        /**
         * @brief signal handler에서 서버를 close하기 위해서 호출되는 함수
         */
        void serverClose();
};

#endif