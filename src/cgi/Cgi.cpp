#include "Cgi.hpp"

Cgi::Cgi() {}
Cgi::~Cgi() {}

bool Cgi::dupSetting(int *in, int *out)
{
    
    if (dup2(in[0], STDIN_FILENO) < 0)
        return (false);
    if (dup2(out[1], STDOUT_FILENO) < 0)
        return (false);
    pipeClose(in);
    pipeClose(out);
    return (true);
}

pid_t Cgi::excute(EnvMap envp, int *in, int *out)
{
    pid_t pid;
    char **env;
    char *cmd[2];

    pid = fork();
    if (pid < 0)
    {
        pipeClose(in);
        pipeClose(out);
        return (-1);
    }
    else if (pid == 0)
    {
        if (!dupSetting(in, out))
            return (-1);        
        env = mapToEnvp(envp);
        cmd[0] = (char *)"/home/seungsch/Webserve/src/cgi/test.py";
        cmd[1] = NULL;
        if (execve(cmd[0], cmd, env) < 0)
        {
            freeSplit(env);
            write(STDERR_FILENO, "CgiError\n", 9);
            return (-1);
        }
    }
    return (pid);
}

void Cgi::pipeClose(int *pipe)
{
    close(pipe[0]);
    close(pipe[1]);
}
