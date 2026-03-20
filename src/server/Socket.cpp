#include "Socket.hpp"

/**
 * @brief 기본 생성자
 */
Socket::Socket()
{
    this->socketFd = -1;
}

/**
 * @brief 파일 디스크립터를 인자로 받는 생성자
 * @param fd 소켓 파일 디스크립터
 */
Socket::Socket(int fd)
{
    this->socketFd = fd;
}

/**
 * @brief 파일 디스크립터와 포트 번호를 인자로 받는 생성자
 * @param fd 소켓 파일 디스크립터
 * @param port 바인딩한 포트 번호
 * @details 서버소켓을 등록할때 사용
 */
Socket::Socket(int fd, in_port_t port)
{
    this->socketFd = fd;
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    this->addr.sin_addr.s_addr = INADDR_ANY;
    memset(this->addr.sin_zero, 0, sizeof(this->addr.sin_zero));
}

/**
 * @brief 파일 디스크립터와 주소 구조체를 인자로 받는 생성자
 * @param fd 소켓 파일 디스크립터
 * @param addr 소켓 주소 구조체 
 * @details 클라이언트의 소켓을 등록할때 사용
 */
Socket::Socket(int fd, struct sockaddr_in addr)
{
    this->socketFd = fd;
    this->addr = addr;
}

/**
 * @brief 소켓의 최근 활동 시간 및 타임아웃 시간을 갱신하는 함수
 */
void Socket::actTimeSet(void)
{
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
}

/**
 * @brief 소켓을 닫고 파일 디스크립터를 정리하는 함수
 */
void Socket::socketClose(void)
{
    close(this->socketFd);
}

/**
 * @brief 소켓의 파일 디스크립터를 반환하는 함수
 * @return 소켓 파일 디스크립터 (const 참조)
 */
const int &Socket::getFd(void) {return (this->socketFd);}

/**
 * @brief 소켓의 주소 정보를 반환하는 함수
 * @return sockaddr_in 구조체 (const 참조)
 */
const struct sockaddr_in &Socket::getAddr(void) const { return (this->addr); }

/**
 * @brief 소켓의 타임아웃 시간을 반환하는 함수
 * @return 설정된 타임아웃 시간 (const 참조)
 */
const time_t &Socket::getTimeOut(void) const { return (this->timeOut); }

/**
 * @brief 소멸자
 * @details 서버와 클라이언트에서 FD에 대해서 close를 하기 때문에 소멸자에서 close 불피요함
 */
Socket::~Socket() {}