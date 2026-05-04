#ifndef CGI_HPP
# define CGI_HPP

#include "main.hpp"
#include "Utils.hpp"

/**
 * @brief CGI(Common Gateway Interface) 실행 및 파이프 통신을 처리하는 클래스
 */
class Cgi
{
    public:
        Cgi();
        ~Cgi();

        /**
         * @brief CGI 프로세스의 표준 입출력을 파이프로 리다이렉션하는 함수
         * @param in 표준 입력으로 사용할 파이프 배열 (크기 2)
         * @param out 표준 출력으로 사용할 파이프 배열 (크기 2)
         * @return 성공 여부 코드
         */
        bool dupSetting(int *in, int *out);

        /**
         * @brief 사용하지 않는 파이프 파일 디스크립터를 닫는 함수
         * @param in 자식 프로스세스에서 dupSetting 후 닫을 파이프 size 2인 int 배열
         */
        void pipeClose(int *in);

        /**
         * @brief CGI 스크립트를 자식 프로세스에서 실행(fork & execve)하는 함수
         * @param envp CGI에 전달할 환경변수 맵
         * @param in 표준 입력 파이프
         * @param out 표준 출력 파이프
         * @return 생성된 자식 프로세스의 PID, 실패 시 -1 반환
         *  나중에 Client request에 맞춰서 cgi실행하도록 변경
         */
        pid_t excute(EnvMap envp, int *in, int *out);
};

#endif