#include "Utils.hpp"

#include <fcntl.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>

EnvMap envpParsing(char **envp)
{
    int i = 0;
    EnvMap env;

    while (envp[i])
    {
        std::string envbuffer(envp[i]);
        size_t pos = envbuffer.find('=');
        if (pos != std::string::npos)
        {
            std::string key = envbuffer.substr(0, pos);
            std::string value = envbuffer.substr(pos + 1);
            env[key] = value;
        }
        i++;
    }
    return env;
}

bool nonblockingSet(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << " Server::nonblockingSet: fcntl(F_GETFL) 실패 " << std::endl;
        return RET_ERROR;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << " Server::nonblockingSet: fcntl(F_GETFL) 실패 " << std::endl;
        return RET_ERROR;
    }
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    return RET_OK;
}

char **mapToEnvp(const EnvMap &env)
{
    int i = 0;
    char **envp = new char *[env.size() + 1];
    std::string envpBuffer;
    EnvMap::const_iterator itr;
    for (itr = env.begin(); itr != env.end(); itr++)
    {
        envpBuffer = itr->first + '=' + itr->second;
        envp[i] = new char[envpBuffer.length() + 1];
        std::strcpy(envp[i], envpBuffer.c_str());
        i++;
    }
    envp[i] = NULL;
    return envp;
}

void freeSplit(char **tmp)
{
    int i = 0;

    if (!tmp)
        return ;
    while (tmp[i])
    {
        delete [] tmp[i];
        i++;
    }
    delete []tmp;
}

std::string ipToString(in_addr_t addr)
{
    uint32_t host = ntohl(addr);
    char buf[16];

    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
        (host >> 24) & 0xFF, (host >> 16) & 0xFF, (host >> 8) & 0xFF, host & 0xFF);
    return std::string(buf);
}

