#ifndef EPOLL_HPP
# define EPOLL_HPP

#include "main.hpp"

/**
 * @brief Linux epoll I/O 다중화 기능을 캡슐화한 클래스
 * @details 
 */
class Epoll
{
    private:
        /**
         * @brief epoll 파일 디스크립터
         * @details epoll 이벤트를 추가 및 삭제를 등록하는 FD
         */
        int epollFd;
        /**
         * @brief 어떤 FD, 이벤트를 등록할지에 대해서 담는 epoll_event구조체
         */
        struct epoll_event event;
        /**
         * @brief 이벤트가 감지되면 감지된 event와 fd에 대해서 복사 받는 event구조체 
         */
        struct epoll_event events[50];
    
    public:
        /**
         * @brief Epoll 객체 생성자. epoll 인스턴스를 생성합니다.
         */
        Epoll();

        /**
         * @brief Epoll 객체 소멸자. epoll 파일 디스크립터를 닫습니다.
         */
        ~Epoll();

        /**
         * @brief epoll 파일 디스크립터를 반환하는 함수
         * @return epoll 파일 디스크립터
         */
        int getEpollFd();

        /**
         * @brief 발생한 이벤트 배열(events)의 특정 인덱스에 접근하는 연산자 오버로딩
         * @param i 인덱스
         * @return 해당 인덱스의 epoll_event 구조체 참조
         */
        epoll_event &operator[] (unsigned int i);

        /**
         * @brief epoll_ctl을 사용하여 이벤트를 제어(추가, 수정, 삭제)하는 함수
         * @param option 제어 옵션 (EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL)
         * @param appendFd 대상 파일 디스크립터
         * @param events 감시할 이벤트 플래그 (EPOLLIN, EPOLLOUT 등)
         * @return 성공 실패 여부
         */
        bool epCtl(int option, int appendFd, u_int64_t events);

        /**
         * @brief epoll_wait을 호출하여 이벤트가 발생할 때까지 대기하는 함수
         * @return 발생한 이벤트의 개수, 실패 시 -1 
         */
        int epWait(void);

        /**
         * @brief epoll_create 실패 시 발생하는 예외 클래스
         */
        class EpollMakeError : public std::exception
		{
			public:
				const char* what() const throw();
		};

        /**
         * @brief epoll_create 실패 시 발생하는 예외 클래스
         */
        class EpollCtlError : public std::exception
		{
			public:
				const char* what() const throw();
		};
};

#endif