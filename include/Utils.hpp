#ifndef UTILS_HPP
# define UTILS_HPP

#include "main.hpp"

/**
 * @brief 특정 파일 디스크립터를 논블로킹(Non-blocking) 모드로 설정하는 함수
 * 
 * fcntl() 함수와 O_NONBLOCK 플래그를 사용하여 I/O 작업(recv, send, accept 등)이 블로킹되지 않고 즉시 반환되도록 만들어 줍니다. epoll과 함께 비동기 I/O를 구현하기 위한 필수 작업입니다.
 * @param fd 설정할 파일 디스크립터
 */
bool nonblockingSet(int fd);

/**
 * @brief envp 전체를 std::map<string, string>으로 파싱해서 반환하는 함수
 * @param envp 환경변수를 담고 있는 char **버퍼
 * @return 파싱된 환경변수들을 담은 EnvMap 객체
 * @details main 함수로 전달된 `char **envp`를 읽어들여 '=' 문자를 기준으로 Key와 Value로 분리해 map 형태로 만듭니다.
 * 서버 초기화 시 단 한 번 호출되며, 이후 모든 CGI 실행 시 환경변수 템플릿의 원본으로 사용됩니다.
 */
EnvMap envpParsing(char **envp);

/**
 * @brief EnvMap 형식의 환경변수 컨테이너를 환경변수 char** 배열로 변환하는 함수 (주로 CGI 프로그램의 envp로 사용)
 * @param env 변환할 환경변수 맵 (const 참조자)
 * @return 동적 할당된 환경변수 char** 배열 (사용 후 freeSplit으로 메모리 해제 필요, 마지막 요소는 NULL)
 * @details CGI 스크립트를 fork & execve 로 실행할 때, 환경변수를 표준 C 문자열 배열 형태로 전달해야 합니다.
 * 이때 C++ 컨테이너인 map의 데이터를 "KEY=VALUE" 형태의 char** 포인터 배열로 직렬화하여 반환해 줍니다.
 */
char **mapToEnvp(const EnvMap &env);

/**
 * @brief 동적 할당된 2차원 문자열 배열(char**)의 메모리를 해제하는 함수
 * @param tmp 메모리를 해제할 char** 배열
 * @details mapToEnvp() 등에서 동적으로 힙(Heap)에 할당한 char** 배열과 그 내부의 각 문자열(char*) 메모리들을 반복문을 통해 안전하게 반납하여 메모리 누수를 방지합니다.
 */
void freeSplit(char **tmp);

#endif