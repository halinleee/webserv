
#include "Epoll.hpp"

/**
 * @brief Epoll 객체 생성자. epoll 인스턴스를 생성합니다.
 * @return Error 발생시 epollFd를 -1로 초기화함
 */
Epoll::Epoll() 
{
    this->epollFd = epoll_create(8192);
}

/**
 * @brief epoll_ctl을 사용하여 이벤트를 제어(추가, 수정, 삭제)하는 함수
 * @param option 제어 옵션 (EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL)
 * @param appendFd 대상 파일 디스크립터
 * @param events 감시할 이벤트 플래그 (EPOLLIN, EPOLLOUT 등)
 * @return 성공 실패 여부
 */
bool Epoll::epollControl(int option, int appendFd, u_int64_t events)
{
    event.data.fd = appendFd;
    event.events = events;
    if (epoll_ctl(this->epollFd, option, appendFd, &event) < 0)
        return (STATUS_ERROR);
    return (STATUS_OK);
}

/**
 * @brief Epoll 객체 소멸자. epoll 파일 디스크립터를 닫습니다.
 */
Epoll::~Epoll()
{
    close(epollFd);
}

/**
 * @brief epoll 파일 디스크립터를 반환하는 함수
 * @return epoll 파일 디스크립터
 */
int Epoll::getEpollFd()
{
    return epollFd;
}

/**
 * @brief 발생한 이벤트 배열(events)의 특정 인덱스에 접근하는 연산자 오버로딩
 * @param i 인덱스
 * @return 해당 인덱스의 epoll_event 구조체 참조
 */
epoll_event &Epoll::operator[] (unsigned int i)
{
    return (events[i]);
}

/**
 * @brief epoll_wait을 호출하여 이벤트가 발생할 때까지 대기하는 함수
 * @return 발생한 이벤트의 개수, 실패 시 -1 
 */
int Epoll::epWait(void)
{
    int eventCount = epoll_wait(this->epollFd, this->events, 50, 0);
    if (eventCount < 0)
        return (-1);
    return (eventCount);
}
