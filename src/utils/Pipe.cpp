#include "Pipe.hpp"
#include <fcntl.h>

Pipe::Pipe()
{
    inPipe[0] = -1;
    inPipe[1] = -1;
    outPipe[0] = -1;
    outPipe[1] = -1;
}

Pipe::~Pipe()
{
    closeSafely(inPipe[0]);
    closeSafely(inPipe[1]);
    closeSafely(outPipe[0]);
    closeSafely(outPipe[1]);
}

void Pipe::closeSafely(FD &fd)
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

bool Pipe::init()
{
    if (pipe(inPipe) < 0)
        return (false);
    if (pipe(outPipe) < 0)
    {
        closeSafely(inPipe[0]);
        closeSafely(inPipe[1]);
        return (false);
    }
    fcntl(inPipe[0],  F_SETFD, FD_CLOEXEC);
    fcntl(inPipe[1],  F_SETFD, FD_CLOEXEC);
    fcntl(outPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(outPipe[1], F_SETFD, FD_CLOEXEC);
    fcntl(inPipe[1],  F_SETFL, fcntl(inPipe[1],  F_GETFL, 0) | O_NONBLOCK);
    fcntl(outPipe[0], F_SETFL, fcntl(outPipe[0], F_GETFL, 0) | O_NONBLOCK);
    return (true);
}

void Pipe::closeChildSide()
{
    closeSafely(inPipe[0]);
    closeSafely(outPipe[1]);
}

void Pipe::detach()
{
    inPipe[0] = -1;
    inPipe[1] = -1;
    outPipe[0] = -1;
    outPipe[1] = -1;
}

FD *Pipe::getInPipeArr()  { return (inPipe); }
FD *Pipe::getOutPipeArr() { return (outPipe); }

FD Pipe::getInWriteFd()  const { return (inPipe[1]); }
FD Pipe::getOutReadFd()  const { return (outPipe[0]); }

void Pipe::closeInWrite()  { closeSafely(inPipe[1]); }
void Pipe::closeOutRead()  { closeSafely(outPipe[0]); }
