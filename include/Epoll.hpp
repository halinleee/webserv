#ifndef EPOLL_HPP
# define EPOLL_HPP

#include "main.hpp"

/**
 * @brief Linux epoll I/O 다중화 기능을 캡슐화한 클래스
 * 
 * 서버에서 다수의 클라이언트 소켓을 효율적으로 관리하기 위해 사용됩니다.
 * 
 * epoll_create, epoll_ctl, epoll_wait 시스템 콜을 객체 지향적으로 다룰 수 있게 해줍니다.
 * 내부적으로 epoll 파일 디스크립터와 이벤트 배열을 관리하며, Server 클래스의 이벤트 루프에서 핵심적인 역할을 수행합니다.
 */
class Epoll
{
    private:
        /**
         * @brief epoll 인스턴스의 파일 디스크립터
         * 
         * epoll_create()를 통해 커널에서 생성된 파일 디스크립터입니다.
         * 다른 소켓 파일 디스크립터들의 이벤트를 감시하고 제어하기 위해 epoll_ctl, epoll_wait 시스템 콜 호출 시 사용됩니다.
         */
        int epollFd;
        /**
         * @brief 이벤트를 등록/수정/삭제할 때 사용하는 임시 epoll_event 구조체
         * 
         * epCtl() 함수 내에서 어떤 파일 디스크립터(fd)에 대해 어떤 이벤트(EPOLLIN, EPOLLOUT 등)를
         * 감시할 것인지 설정하여 epoll_ctl() 시스템 콜에 전달하는 용도로 사용됩니다.
         */
         epoll_event event;
        /**
         * @brief 발생한 이벤트들을 운영체제로부터 복사받는 epoll_event 구조체 배열
         * 
         * epWait() 함수 호출 시, 운영체제가 감지한 I/O 이벤트들의 정보(발생한 fd와 이벤트 종류)가
         * 이 배열에 채워집니다. 최대 50개의 이벤트를 한 번에 가져올 수 있으며, Server 클래스의 이벤트 루프에서 이 배열을 순회합니다.
         */
         epoll_event events[50];
    
    public:
        /**
         * @brief Epoll 객체 생성자. epoll 인스턴스를 생성합니다.
         * 
         * 내부적으로 epoll_create() 시스템 콜을 호출하여 epollFd를 초기화합니다.
         * 생성 실패 시 FD를 -1로 초기화하며, 주로 서버 초기화 단계에서 단 한 번 호출됩니다.
         */
        Epoll();

        /**
         * @brief Epoll 객체 소멸자. 사용 중인 epoll 파일 디스크립터를 닫습니다.
         * 
         * 객체가 소멸될 때 close() 시스템 콜을 통해 epollFd를 닫아 커널 자원 누수를 방지합니다.
         */
        ~Epoll();

        /**
         * @brief epoll 파일 디스크립터를 반환하는 함수
         * 
         * 외부에서 직접 epollFd를 참조해야 할 경우 사용됩니다.
         * @return 현재 생성되어 있는 epoll 인스턴스의 파일 디스크립터 (int)
         */
        int getEpollFd();

        /**
         * @brief 발생한 이벤트 배열(events)의 특정 인덱스에 접근하는 연산자 오버로딩
         
         * Server::eventProcess() 함수 내에서 epWait() 반환 후 for 루프를 통해 발생한 각 이벤트(fd 및 이벤트 플래그)를 배열 인덱싱 하듯이 편리하게 조회하기 위해 사용됩니다.
         * @param i 인덱스
         * @return 해당 인덱스의 epoll_event 구조체 참조
         */
        epoll_event &operator[] (unsigned int i);

        /**
         * @brief epoll_ctl을 사용하여 이벤트를 제어(추가, 수정, 삭제)하는 함수
         * 
         * Server 클래스에서 새로운 소켓 연결이 발생하거나(ADD), 클라이언트 요청 수신 후 응답을 보낼 준비가 되었을 때(MOD), 또는 연결이 종료되었을 때(DEL) epoll에 FD를 등록/변경/해제하기 위해 호출합니다.
         * @param option 제어 옵션 (EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL)
         * @param appendFd 대상 파일 디스크립터
         * @param events 감시할 이벤트 플래그 (EPOLLIN, EPOLLOUT 등)
         * @return 작업의 성공 여부
         */
        bool epollControl(int option, int appendFd, u_int64_t events);

        /**
         * @brief epoll_wait을 호출하여 이벤트가 발생할 때까지 대기하는 함수
         *          
         * * Server::eventProcess()의 무한 루프 첫 부분에서 호출되어, 등록된 FD들 중 하나라도 I/O 이벤트가 발생할 때까지(또는 타임아웃) 스레드를 블로킹 상태로 대기시킵니다.
         * @return 발생한 이벤트의 개수, 실패 시 -1
         */
        int epWait(void);
};

#endif