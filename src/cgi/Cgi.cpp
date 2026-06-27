#include "Cgi.hpp"
#include <cstdlib>
#include <sstream>
#include <arpa/inet.h>

Cgi::Cgi() {}
Cgi::Cgi(std::string prefix, LocationConfig location) : cgiLocation(location), cgiPrefix(prefix) {}
Cgi::~Cgi() {}

bool Cgi::dupSetting(int *in, int *out)
{
    
    if (dup2(in[0], STDIN_FILENO) < 0)
        return false;
    if (dup2(out[1], STDOUT_FILENO) < 0)
        return false;
    pipeClose(in);
    pipeClose(out);
    return true;
}

pid_t Cgi::excute(Client *client, EnvMap envp, int *in, int *out)
{
    pid_t pid;
    char **env;
    char *cmd[3];
    Request request = client->getRequest();

    pid = fork();
    if (pid < 0)
    {
        pipeClose(in);
        pipeClose(out);
        return -1;
    }
    else if (pid == 0)
    {
        if (!dupSetting(in, out))
            exit (-1);
        envAppend(client, envp, request);
        env = mapToEnvp(envp);
        std::string path = client->getRequest().path.substr(this->cgiPrefix.length());
        std::string cgiPath = this->cgiLocation.getRoot() + path;
        cmd[0] = const_cast<char *>(this->cgiLocation.getCgiPath().c_str()); 
        cmd[1] = const_cast<char *>(cgiPath.c_str());
        cmd[2] = NULL;
        if (execve(cmd[0], cmd, env) < 0)
        {
            freeSplit(env);
            write(STDERR_FILENO, "CgiError\n", 9);
            exit (-1);
        }
    }
    return pid;
}

static std::string methodToString(HttpMethod method)
{
    switch (method)
    {
        case METHOD_GET: return "GET";
        case METHOD_POST: return "POST";
        case METHOD_DELETE: return "DELETE";
        default: return "";
    }
}

void Cgi::envAppend(Client *client, EnvMap &envp, Request request)
{
    std::stringstream ss;

    envp["REQUEST_METHOD"] = methodToString(request.method);
    envp["SCRIPT_NAME"] = request.path;
    envp["QUERY_STRING"] = request.query;
    envp["SERVER_PROTOCOL"] = "HTTP/1.1";
    envp["GATEWAY_INTERFACE"] = "CGI/1.1";
    envp["SERVER_NAME"] = request.host;
    envp["REMOTE_ADDR"] = inet_ntoa(client->getSocket().getAddr().sin_addr);
    ss << request.port;
    envp["SERVER_PORT"] = ss.str();
    ss.clear();

    if (request.contentLength >= 0)
    {
        ss << request.contentLength;
        envp["CONTENT_LENGTH"] = ss.str();
    }

    std::map<std::string, std::string>::const_iterator it = request.headers.find("content-type");
    if (it != request.headers.end())
        envp["CONTENT_TYPE"] = it->second;
}

void Cgi::pipeClose(int *pipe)
{
    close(pipe[0]);
    close(pipe[1]);
}
