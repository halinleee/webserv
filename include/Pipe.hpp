#ifndef PIPE_HPP
#define PIPE_HPP

#include "type.hpp"
#include <unistd.h>

/**
 * @brief CGI 통신용 파이프 쌍(inPipe, outPipe)을 RAII로 관리하는 클래스
 *
 * inPipe : 부모(서버) → 자식(CGI) 방향. [0]=자식 stdin, [1]=부모 쓰기 끝
 * outPipe: 자식(CGI) → 부모(서버) 방향. [0]=부모 읽기 끝, [1]=자식 stdout
 */
class Pipe
{
    private:
        FD inPipe[2];
        FD outPipe[2];

        void closeSafely(FD &fd);

    public:
        Pipe();
        ~Pipe();

        bool init();
        void closeChildSide();
        void detach();

        FD *getInPipeArr();
        FD *getOutPipeArr();

        FD getInWriteFd() const;
        FD getOutReadFd() const;

        void closeInWrite();
        void closeOutRead();
};

#endif
