#include "./include/Config.hpp"
#include <iostream>

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
    std::cout << "    location: " << prefix << "\n";
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
    std::cout << "== server " << port << " ==\n";
    std::cout << "  client_max_body_size: " << srv.getClientMaxBodySize() << "\n";

    // error_pages
    std::cout << "  error_pages:\n";
    std::map<unsigned int, std::string> eps = srv.getErrorPages(); // getter가 value 반환이라 복사해서 사용
    if (eps.empty())
        std::cout << "    (none)\n";
    for (std::map<unsigned int, std::string>::const_iterator it = eps.begin(); it != eps.end(); ++it)
        std::cout << "    " << it->first << " -> " << it->second << "\n";

    // locations
    std::cout << "  locations:\n";
    std::map<std::string, LocationConfig> locs = srv.getLocations(); // value 반환
    if (locs.empty())
        std::cout << "    (none)\n";
    for (std::map<std::string, LocationConfig>::const_iterator it = locs.begin(); it != locs.end(); ++it)
        dumpLocation(it->first, it->second);

    std::cout << "\n";
}

int main()
{
    // Config 생성자에서 ./config/webserv.conf 를 열고 파싱합니다.
    // (에러/상태 메시지는 Config 생성자 내부에서 출력 중)
    Config config;

    // 전체 데이터 덤프
    std::map<unsigned int, ServerConfig> servers = config.getConfig(); // 현재 getter가 by-value라 복사됨
    if (servers.empty())
    {
        std::cout << "No servers parsed.\n";
        return 0;
    }

    if (config.getStatusMessage() != "ok")
        return 0;
    std::cout << "\n==== Parsed Config Dump ====\n";
    for (std::map<unsigned int, ServerConfig>::const_iterator it = servers.begin(); it != servers.end(); ++it)
        dumpServer(it->first, it->second);

    return 0;
}