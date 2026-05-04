#include "Socket.hpp"

/**
 * @brief 기본 생성자
 * @details 멤버 변수인 socketFd를 유효하지 않은 값(-1)으로 초기화합니다. 임시 객체 생성 시 사용합니다.
 */
Socket::Socket()
{
    this->socketFd = -1;
    this->timeAct = 0;
    this->timeOut = 0;
}

/**
 * @brief 파일 디스크립터와 포트 번호를 인자로 받는 생성자
 * 
 * sin_family = IP주소체계, sin_port = port번호, sin_addr.s_addr = IP주소 sin_zero = 패딩 데이터
 * @param fd 소켓 파일 디스크립터
 * @param port 바인딩한 포트 번호
 * @details 서버의 리스닝 소켓을 설정할 때 호출됩니다. 입력받은 포트를 네트워크 바이트 순서(htons)로 변환하고,
 * INADDR_ANY를 통해 모든 네트워크 인터페이스에서 들어오는 연결을 수신할 수 있도록 sockaddr_in 구조체를 세팅합니다.
 */
Socket::Socket(int fd, in_port_t port)
{
    this->socketFd = fd;
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
    memset(this->addr.sin_zero, 0, sizeof(this->addr.sin_zero));
}

/**
 * @brief 파일 디스크립터와 주소 구조체를 인자로 받는 생성자
 * 
 * 클라이언트 연결을 수락(accept)한 후 얻게 된 새로운 통신 FD와 클라이언트의 주소 정보(IP, Port)를 Socket 객체에 보관하여 관리 목적으로 사용할 때 호출됩니다.
 * @param fd 소켓 파일 디스크립터
 * @param addr 소켓 주소 구조체
 * @todo 소켓 타임에 관련한 로직 추가
 */
Socket::Socket(int fd, struct sockaddr_in addr)
{
    this->socketFd = fd;
    this->addr = addr;
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
}

/**
 * @brief 소켓의 최근 활동 시간 및 타임아웃 시간을 갱신하는 함수
 * @details 클라이언트가 새로운 요청을 보내거나 의미 있는 I/O 동작을 수행할 때마다 호출되어, 타임아웃으로 인한 강제 연결 종료 시점을 뒤로 미룹니다.
 */
void Socket::actTimeSet(void)
{
    this->timeAct = std::time(NULL);
    this->timeOut = this->timeAct + DEFAULT_TIMEOUT;
}

/**
 * @brief 소켓의 파일 디스크립터를 반환하는 함수
 * @return 소켓 파일 디스크립터 (const 참조)
 * @details Epoll 등록이나 recv(), send() 등 시스템 콜을 호출할 때 필요한 커널 수준의 식별자(FD)를 제공합니다.
 */
const int &Socket::getFd(void) const {return (this->socketFd);}

/**
 * @brief 소켓의 주소 정보를 반환하는 함수
 * @return sockaddr_in 구조체 (const 참조)
 * @details 로깅 처리, CGI 환경 변수 세팅 등 클라이언트가 접속한 IP 주소 혹은 포트 정보가 필요할 때 사용됩니다.
 */
const struct sockaddr_in &Socket::getAddr(void) const { return (this->addr); }

/**
 * @brief 소켓의 타임아웃 시간을 반환하는 함수
 * @return 설정된 타임아웃 시간 (const 참조)
 * @details 서버의 백그라운드 루프 등에서 현재 시간과 비교하여 설정된 만료 시간(timeOut)이 지났는지 검사하고 정리에 사용됩니다.
 */
const time_t &Socket::getTimeOut(void) const { return (this->timeOut); }

/**
 * @brief 소멸자
 * @details 만약 유효한 FD를 보유하고 있다면, 소멸되는 시점에 close()를 수행하여 커널 자원(파일 디스크립터)을 시스템에 반납합니다.
 * 객체 소멸 시점에서 자원 회수가 보장되는 RAII 원칙을 따릅니다.
 */
Socket::~Socket() 
{
    if (this->socketFd != -1)
        close(this->socketFd);
}