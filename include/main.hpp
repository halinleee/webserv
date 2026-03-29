#ifndef MAIN_HPP
# define MAIN_HPP

#include "type.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <exception>
#include <cstring>

#include <ctime>// 시간 관련 함수 사용 (std::time 등)
#include <unistd.h>     // fork, execve, pipe, dup, dup2, chdir, close, read, write, access
#include <fcntl.h>      // open, fcntl 등 파일 제어

/* 데이터 타입 및 상태 관련 */
#include <sys/types.h>  // pid_t, size_t 등 시스템 데이터 타입 정의

/* 네트워크 (Socket) 관련 */
#include <sys/socket.h> // socket, socketpair, accept, listen, send, recv, bind, connect, setsockopt, getsockname
#include <netinet/in.h> // htons, htonl, ntohs, ntohl (바이트 순서 변환)
#include <sys/epoll.h>  // epoll_create, epoll_ctl, epoll_wait (Linux 전용)

#endif