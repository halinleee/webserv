#include "Config.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "Util.hpp"

#include <netinet/in.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static std::string methodToString(HttpMethod m)
{
    switch (m)
    {
        case METHOD_GET: return "GET";
        case METHOD_POST: return "POST";
        case METHOD_DELETE: return "DELETE";
    }
    return "UNKNOWN";
}

static void dumpLocation(const std::string& prefix, const LocationConfig& loc)
{
    std::cout << "  [location] " << prefix << "\n";
    std::cout << "    root        : " << loc.getRoot() << "\n";
    std::cout << "    index       : " << loc.getIndex() << "\n";
    std::cout << "    autoindex   : " << (loc.getAutoIndex() ? "on" : "off") << "\n";

    std::cout << "    methods     :";
    for (std::set<HttpMethod>::const_iterator it = loc.getMethods().begin();
         it != loc.getMethods().end(); ++it)
    {
        std::cout << " " << methodToString(*it);
    }
    std::cout << "\n";

    std::cout << "    upload_dir  : " << loc.getUploadDir() << "\n";
    std::cout << "    return      : " << loc.getRedirectPath() << "\n";
    std::cout << "    cgi_ext     : " << loc.getCgiExtension() << "\n";
    std::cout << "    cgi_path    : " << loc.getCgiPath() << "\n";
}

static void dumpServer(in_port_t port, const ServerConfig& srv)
{
    std::cout << "==============================\n";
    std::cout << "[server] port: " << port << "\n";
    std::cout << "  client_max_body_size : " << srv.getClientMaxBodySize() << "\n";
    std::cout << "  keepalive_timeout    : " << srv.getKeepAliveTimeout() << "\n";

    // error_page
    const std::map<size_t, std::string>& eps = srv.getErrorPages();
    std::cout << "  error_pages (" << eps.size() << ")\n";
    for (std::map<size_t, std::string>::const_iterator it = eps.begin();
         it != eps.end(); ++it)
    {
        std::cout << "    " << it->first << " -> " << it->second << "\n";
    }

    // locations
    const std::map<std::string, LocationConfig>& locs = srv.getLocations();
    std::cout << "  locations (" << locs.size() << ")\n";
    for (std::map<std::string, LocationConfig>::const_iterator it = locs.begin();
         it != locs.end(); ++it)
    {
        dumpLocation(it->first, it->second);
    }
    std::cout << "==============================\n";
}

static void printServerList(const std::map<in_port_t, ServerConfig>& servers)
{
    std::cout << "Available servers:\n";
    size_t idx = 1;
    for (std::map<in_port_t, ServerConfig>::const_iterator it = servers.begin();
         it != servers.end(); ++it, ++idx)
    {
        std::cout << "  [" << idx << "] port " << it->first
                  << " (status: " << it->second.getStatusMessage() << ")\n";
    }
}

static bool selectServer(
    const std::map<in_port_t, ServerConfig>& servers,
    const std::string& input,
    std::map<in_port_t, ServerConfig>::const_iterator& outIt)
{
    // 1) 포트로 찾기
    size_t asNumber = 0;
    if (toInt(input, asNumber))
    {
        // a) index(1..N)로 들어온 경우도 처리
        if (asNumber >= 1 && asNumber <= servers.size())
        {
            size_t i = 1;
            for (std::map<in_port_t, ServerConfig>::const_iterator it = servers.begin();
                 it != servers.end(); ++it, ++i)
            {
                if (i == asNumber)
                {
                    outIt = it;
                    return true;
                }
            }
        }

        // b) port로 들어온 경우
        in_port_t port = static_cast<in_port_t>(asNumber);
        std::map<in_port_t, ServerConfig>::const_iterator it = servers.find(port);
        if (it != servers.end())
        {
            outIt = it;
            return true;
        }
    }
    return false;
}

int main()
{
    Config config;
    std::cout << "[Config status] " << config.getStatusMessage() << "\n";

    const std::map<in_port_t, ServerConfig>& servers = config.getConfig();
    if (servers.empty())
    {
        std::cout << "(no servers)\n";
        return 0;
    }

    while (true)
    {
        printServerList(servers);
        std::cout << "\n";
        std::cout << "Select server by [index] or [port] (q to quit): ";

        std::string line;
        if (!std::getline(std::cin, line))
            break;

        if (line == "q" || line == "Q" || line == "quit" || line == "exit")
            break;

        std::map<in_port_t, ServerConfig>::const_iterator it = servers.end();
        if (!selectServer(servers, line, it))
        {
            std::cout << "Invalid selection. Try again.\n\n";
            continue;
        }

        dumpServer(it->first, it->second);
        std::cout << "\n";
    }

    return 0;
}