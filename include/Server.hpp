#ifndef SERVER_HPP
# define SERVER_HPP

#include "main.hpp"
#include "Epoll.hpp"
#include "Utils.hpp"
#include "Socket.hpp"
#include "Client.hpp"

/**
 * @brief 웹 서버의 전반적인 동작과 클라이언트 연결, 이벤트를 관리하는 핵심 클래스
 *  소켓 관리, Epoll 이벤트 루프, CGI 실행, 클라이언트 요청 및 응답 처리를 수행합니다.
 */
class Server
{
    private:
        /**
         * @var serverSocket 
         * @brief 서버의 소켓을 담고 있는 <int, Socket> 맵
         * 
         * key = 서버의 FD, Socket = 서버의 FD, addr, time(마지막 사용시간)을 가지고 있는 클래스
         */
        Socket *serverSocket;
        /**
         * @var response CGI에서 읽은 정보를 사용한 답변을 가지고 있는 String변수
         * @brief CGI프로그램에서 pipe를 통해서 나온 Response답변을 clientResponse함수까지 옮기기 위해서 사용
        */
        std::string response;
        /**
         * @var client 
         * @brief 클라이언트의 정보를 담고 있는 <int, Client> 맵
         * 
         * key = client소켓 FD, value = 클라이언트 정보 및 관련 함수를 가지고 있는 클래스
        */
        ClientMap client;
        /**
         * @var pipeToClientMap
         * @brief 파이프의 FD와 클라이언트의 FD를 매핑해서 가지고 있는 <int, int>맵
         *  
         * Epoll 사용시 pipe의 FD가 epollFD로 나오기 때문에 clinet객체에서 순회하면서 찾기 힘들어서 이 맵을 사용하여 client의 FD를 확인
        */
        IntMap pipeToClientMap;
        /**
         * @var env 
         * @brief main함수의 기본적인 환경변수를 가지고 있는 <string, string>맵
         * 
         * CGI프로그램 구동시 환경변수가 필요한데 client에 복사할때 사용
         */
        EnvMap env;
        
    public:
        /**
         * @brief Server 생성자. 초기 환경변수를 파싱하여 저장합니다.
         * @param envp 메인 함수에서 전달받은 환경변수
         */
        Server(char **envp);

        /**
         * @brief Client 맵과 serverSocket으로 할당받은 자원회수
         */
        ~Server();

        /**
         * @brief 새로운 서버 소켓을 생성하고 Epoll에 등록하는 함수
         * @param port 바인딩할 포트 번호
         * @param epoll 이벤트를 관리할 Epoll 객체
         */
        void serverAdd (in_port_t port, Epoll &epoll);

        /**
         * @brief Epoll 이벤트를 감지하고 종류에 따라 분기 처리하는 메인 이벤트 루프 함수
         * @param epoll 이벤트를 관리할 Epoll 객체
         */
        void eventProcess(Epoll &epoll);

        /**
         * @brief 서버 소켓의 바인딩 및 리슨을 설정하는 함수
         * @param serverSocket 설정할 서버 Socket 객체
         */
        void serverSetting(Socket *serverSocket);

        /**
         * @brief 클라이언트의 연결 요청을 수락하고 Client 객체를 생성 및 Epoll에 등록하는 함수
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param socket 연결 요청을 받은 서버 Socket 객체
         */
        void clientAccept(Epoll &epoll, Socket *socket); //차후에 서버에 접속하는 클라이언트 정보를 가공할 일이 있으면 수정 필요

        /**
         * @brief 클라이언트의 데이터를 읽어들이고 요청을 파싱하는 함수
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param socket 데이터를 전송한 클라이언트 Socket 객체
         */
        void clientRequest(Epoll &epoll, Socket *socket); // 차후에 클라이언트 요청이 들어오는걸 파싱하는 로직이 들어가야함(현재 프린트만 하도록 동작)

        /**
         * @brief 클라이언트에게 응답 데이터를 전송하는 함수
         * @param epoll 이벤트를 관리할 Epoll 객체
         * @param socket 응답을 보낼 클라이언트 Socket 객체
         */
        void clientResponse(Epoll &epoll, Socket *socket);

        /**
         * @brief 특정 파일 디스크립터를 논블로킹(Non-blocking) 모드로 설정하는 함수
         * @param fd 설정할 파일 디스크립터
         */
        void nonblockingSet(int fd);

        /**
         * @brief CGI 프로그램을 실행하고 파이프를 설정하는 함수
         * @param eventSocket 클라이언트 소켓의 FD
         * @param epoll 이벤트를 관리할 Epoll 객체
         */
        bool cgiRun(Epoll &epoll, int eventSocket);

        /**
         * @brief CGI 프로세스로부터 데이터를 읽어들이는 함수
         */
        void cgiPipeRead(Epoll &epoll, Socket *socket);

        /**
         * @brief CGI 프로세스로 데이터를 전송(쓰기)하는 함수
         */
        void cgiPipeWrite(Epoll &epoll, Socket *socket);

        /**
         * @brief 특정 클라이언트의 연결을 종료하고 자원을 해제하는 함수
         * @param deleteFd 삭제할 클라이언트의 소켓 FD
         */
        void clientDel(int deleteFd);
        
        /**
         * @brief 소켓 생성 실패 시 발생하는 예외 클래스
         */
        class SocketMakeError : public std::exception
		{
			public:
				const char* what() const throw();
		};
};

#endif