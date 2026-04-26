#include "./include/Config.hpp"
#include <iostream>
#include <sstream>

static const char* methodToStr(HttpMethod m)
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
    std::cout << "    location " << prefix << "\n";
    std::cout << "      root: " << loc.getRoot() << "\n";
    std::cout << "      index: " << loc.getIndex() << "\n";
    std::cout << "      autoindex: " << (loc.getAutoIndex() ? "on" : "off") << "\n";

    std::cout << "      methods:";
    const std::set<HttpMethod>& ms = loc.getMethods();
    for (std::set<HttpMethod>::const_iterator it = ms.begin(); it != ms.end(); ++it)
        std::cout << " " << methodToStr(*it);
    std::cout << "\n";

    std::cout << "      upload_dir: " << loc.getUploadDir() << "\n";
    std::cout << "      return: " << loc.getRedirectStatusCode() << " " << loc.getRedirectPath() << "\n";
    std::cout << "      cgi_ext: " << loc.getCgiExtension() << " " << loc.getCgiPath() << "\n";
}

static void dumpServer(unsigned int port, const ServerConfig& srv)
{
    std::cout << "server " << port << "\n";
    std::cout << "  statusMessage: " << srv.getStatusMessage() << "\n";
    std::cout << "  client_max_body_size: " << srv.getClientMaxBodySize() << "\n";

    std::cout << "  error_pages:\n";
    std::map<unsigned int, std::string> eps = srv.getErrorPages();
    if (eps.empty())
        std::cout << "    (none)\n";
    for (std::map<unsigned int, std::string>::const_iterator it = eps.begin(); it != eps.end(); ++it)
        std::cout << "    " << it->first << " -> " << it->second << "\n";

    std::cout << "  locations:\n";
    std::map<std::string, LocationConfig> locs = srv.getLocations();
    if (locs.empty())
        std::cout << "    (none)\n";
    for (std::map<std::string, LocationConfig>::const_iterator it = locs.begin(); it != locs.end(); ++it)
        dumpLocation(it->first, it->second);

    std::cout << "\n";
}

static bool parseUint(const std::string& s, unsigned int& out)
{
    std::stringstream ss(s);
    unsigned long v = 0;
    char extra = 0;

    ss >> v;
    if (ss.fail())
        return false;

    // 남는 문자 있으면 실패(공백은 허용)
    if (ss >> extra)
        return false;

    if (v > 65535)
        return false;

    out = static_cast<unsigned int>(v);
    return true;
}

int main()
{
    Config config;

    std::cout << "[Config status] " << config.getStatusMessage() << "\n";

    std::map<unsigned int, ServerConfig> servers = config.getConfig();
    if (servers.empty())
    {
        std::cout << "(no servers)\n";
        return 0;
    }

    // 1) 포트 목록만 출력
    std::cout << "Available ports:";
    for (std::map<unsigned int, ServerConfig>::const_iterator it = servers.begin();
         it != servers.end(); ++it)
    {
        std::cout << " " << it->first;
    }
    std::cout << "\n";

    // 2) 사용자 입력 받기
    std::cout << "Enter port: ";
    std::string line;
    if (!std::getline(std::cin, line))
        return 0;

    unsigned int port = 0;
    if (!parseUint(line, port))
    {
        std::cout << "Invalid port input.\n";
        return 0;
    }

    // 3) 해당 포트 서버만 출력
    std::map<unsigned int, ServerConfig>::const_iterator it = servers.find(port);
    if (it == servers.end())
    {
        std::cout << "No server for port " << port << ".\n";
        return 0;
    }

    dumpServer(it->first, it->second);
    return 0;
}