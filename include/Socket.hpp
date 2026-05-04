#ifndef SOCKET_HPP
# define SOCKET_HPP
# define DEFAULT_TIMEOUT 50

#include "main.hpp"

/**
 * @brief 네트워크 소켓을 관리하는 클래스
 * @details 파일 디스크립터(FD), 주소 정보(sockaddr_in), 타임아웃 등을 캡슐화하여 단일 소켓의 생명주기와 상태를 관리합니다.
 * 서버의 리스닝 소켓뿐만 아니라 클라이언트와 연결된 통신 소켓 모두 이 클래스를 통해 객체화되어 관리되며,
 * 각 소켓의 정보 은닉 및 안전한 자원 관리를 위해 사용됩니다.
 */
class Socket
{
    private:
        /**
         * @brief OS로부터 할당받은 실제 소켓 파일 디스크립터
         * @details bind, listen, accept, recv, send 등 시스템 콜의 대상으로 사용됩니다.
         */
        int socketFd;
        /**
         * @brief 소켓 주소 정보
         * @details 주소 체계(sin_family), 포트 번호(sin_port), IP 주소(sin_addr) 정보가 담겨있습니다.
         * 서버 소켓의 경우 바인딩할 때 사용되며, 클라이언트 소켓의 경우 연결된 클라이언트의 정보를 보관합니다.
         */
        struct sockaddr_in addr;
        /**
         * @param timeAct
         * @brief 클라이언트가 http요청을 보낸 시간
         * 
         * keep-alive에 대해서 구현하기 위해서 필요하고 http request를 할떄마다 갱신 필요
         */
        time_t timeAct;
        /**
         * @param timeOut
         * @brief 클라이언트가 http요청을 보내고 통신이 유지되는 시간
         * 
         * keep-alive에 대해서 구현하기 위해서 필요하고 http request를 할떄마다 갱신 필요
         */
        time_t timeOut;
    
    public:
        /**
         * @brief 기본 생성자
         * @details FD를 -1로 초기화합니다. 주로 빈 소켓 객체를 임시로 생성할 때 사용됩니다.
         */
        Socket();

        /**
         * @brief 파일 디스크립터와 포트 번호를 인자로 받는 생성자
         * @param fd 소켓 파일 디스크립터
         * @param port 바인딩한 포트 번호
         * @details Server 클래스에서 새로운 서버 리스닝 소켓을 생성하고 포트와 결합할 때 호출됩니다.
         */
        Socket(int fd, in_port_t port);

        /**
         * @brief 파일 디스크립터와 주소 구조체를 인자로 받는 생성자
         * @param fd 소켓 파일 디스크립터
         * @param addr 소켓 주소 구조체
         * @details accept()를 통해 새로 연결된 클라이언트 소켓의 정보를 래핑하여 Client 객체에 넘겨줄 때 사용됩니다.
         */
        Socket(int fd, struct sockaddr_in addr);

        /**
         * @brief 소멸자
         * @details 내부적으로 close(socketFd)를 호출하여 파일 디스크립터를 안전하게 닫고 자원을 커널에 반환합니다.
         */
        ~Socket();

        /**
         * @brief 소켓의 파일 디스크립터를 반환하는 함수
         * @return 소켓 파일 디스크립터 (const 참조)
         * @details Epoll 등록, recv, send 등 FD가 직접 필요한 커널 시스템 콜에 전달하기 위해 사용됩니다.
         */
        const int &getFd(void) const;

        /**
         * @brief 소켓의 주소 정보를 반환하는 함수
         * @return sockaddr_in 구조체 (const 참조)
         * @details 클라이언트의 IP를 로깅하거나 환경변수(CGI)에 설정하는 등 주소 정보가 필요할 때 호출됩니다.
         */
        const struct sockaddr_in &getAddr(void) const;

        /**
         * @brief 소켓의 타임아웃 시간을 반환하는 함수
         * @return 설정된 타임아웃 시간 (const 참조)
         * @details 서버가 장기 미활동 클라이언트를 정리(Timeout 처리)할지 판별하는 기준 시간으로 사용됩니다.
         */
        const time_t &getTimeOut(void) const;

        /**
         * @brief 소켓의 최근 활동 시간 및 타임아웃 시간을 갱신하는 함수
         * @details 클라이언트로부터 새로운 요청(recv)이 들어올 때마다 호출되어, 연결 유지 기한을 연장합니다.
         */
        void actTimeSet(void);
};



#endif