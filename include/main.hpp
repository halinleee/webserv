#ifndef MAIN_HPP
# define MAIN_HPP

#include "type.hpp"

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <exception>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>

#include <ctime> // time 불러오기 gettimefday
#include <stdio.h>      // printf, perror 등 표준 입출력
#include <stdlib.h>     // exit, malloc, free 등 일반 유틸리티
#include <string.h>     // strerror, memset, strlen 등 문자열 조작
#include <unistd.h>     // fork, execve, pipe, dup, dup2, chdir, close, read, write, access
#include <errno.h>      // errno 변수 사용을 위한 헤더
#include <fcntl.h>      // open, fcntl 등 파일 제어
#include <signal.h>     // kill, signal 등 시그널 처리
#include <dirent.h>     // opendir, readdir, closedir 등 디렉토리 핸들링
#include <netdb.h>      // getaddrinfo, freeaddrinfo, getprotobyname, gai_strerror

/* 데이터 타입 및 상태 관련 */
#include <sys/types.h>  // pid_t, size_t 등 시스템 데이터 타입 정의
#include <sys/stat.h>   // stat 함수 및 파일 상태 구조체
#include <sys/wait.h>   // waitpid 함수

/* 네트워크 (Socket) 관련 */
#include <sys/socket.h> // socket, socketpair, accept, listen, send, recv, bind, connect, setsockopt, getsockname
#include <netinet/in.h> // htons, htonl, ntohs, ntohl (바이트 순서 변환)
#include <arpa/inet.h>  // 네트워크 주소 변환 유틸리티

/* I/O 다중화 (운영체제에 따라 선택적으로 사용) */
#include <sys/select.h> // select
#include <poll.h>       // poll
#include <sys/epoll.h>  // epoll_create, epoll_ctl, epoll_wait (Linux 전용)

#endif