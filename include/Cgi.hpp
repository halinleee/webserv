#ifndef CGI_HPP
# define CGI_HPP

#include "type.hpp"
#include "Utils.hpp"
#include "LocationConfig.hpp"
#include "Client.hpp"

#include <sys/types.h>
#include <unistd.h>

class Cgi
{
    private:
        LocationConfig cgiLocation;
        std::string cgiPrefix;
    public:
        Cgi();
        Cgi(std::string prefix, LocationConfig location);
        ~Cgi();

        bool dupSetting(int *in, int *out);

        void pipeClose(int *in);

        pid_t excute(Client *client, EnvMap envp, int *in, int *out);

        void envAppend(Client *client, EnvMap &envp, Request request, const std::string &scriptPath);
        std::string changeHeaderEnvkey(std::string name);
};

#endif