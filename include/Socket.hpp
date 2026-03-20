#ifndef SOCKET_HPP
# define SOCKET_HPP
# define DEFAULT_TIMEOUT 60

#include "main.hpp"

/**
 * @brief 네트워크 소켓을 관리하는 클래스
 *  파일 디스크립터(FD), 주소 정보, 타임아웃 등을 캡슐화하여 관리합니다.
 */
class Socket
{
    private:
        /**
         * @brief 소켓 파일 디스크립터
         */
        int socketFd;
        /**
         * @brief 소켓 주소 정보
         * 
         * addr 안에는 주소와 port, 주소체계에 대한 정보가 담긴다.
         * 
         * sin_family = 주소체계, sin_port = 포트번호, sin_addr = ip주소, sin_zero = ipv6로 들어왔을 때를 대비한 패딩 데이터
         */
        struct sockaddr_in addr;
        time_t timeAct;
        time_t timeOut;
    
    public:
        /**
         * @brief 기본 생성자
         */
        Socket();

        /**
         * @brief 파일 디스크립터를 인자로 받는 생성자
         * @param fd 소켓 파일 디스크립터
         */
        Socket(int fd);

        /**
         * @brief 파일 디스크립터와 포트 번호를 인자로 받는 생성자
         * @param fd 소켓 파일 디스크립터
         * @param port 바인딩한 포트 번호
         *  서버소켓을 등록할때 사용
         */
        Socket(int fd, in_port_t port);

        /**
         * @brief 파일 디스크립터와 주소 구조체를 인자로 받는 생성자
         * @param fd 소켓 파일 디스크립터
         * @param addr 소켓 주소 구조체
         *  클라이언트의 소켓을 등록할때 사용
         */
        Socket(int fd, struct sockaddr_in addr);

        /**
         * @brief SocketFd에 대해서 close 후 소멸
         */
        ~Socket();

        /**
         * @brief 소켓의 파일 디스크립터를 반환하는 함수
         * @return 소켓 파일 디스크립터 (const 참조)
         */
        const int &getFd(void) const;

        /**
         * @brief 소켓의 주소 정보를 반환하는 함수
         * @return sockaddr_in 구조체 (const 참조)
         */
        const struct sockaddr_in &getAddr(void) const;

        /**
         * @brief 소켓의 타임아웃 시간을 반환하는 함수
         * @return 설정된 타임아웃 시간 (const 참조)
         */
        const time_t &getTimeOut(void) const;

        /**
         * @brief 소켓의 최근 활동 시간 및 타임아웃 시간을 갱신하는 함수
         */
        void actTimeSet(void);
};



#endif