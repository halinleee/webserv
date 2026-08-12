#ifndef PIPE_HPP
#define PIPE_HPP

#include "type.hpp"

#include <unistd.h>

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

