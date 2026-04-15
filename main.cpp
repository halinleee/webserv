#include "./include/Config.hpp"

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

int main()
{
	std::ifstream configFile("./config/webserv.conf");
	if (!configFile.is_open())
		return (-1);
	//.conf 확장자가 맞는지 확인하는 거 추가

	Config config;
	//유효성 검사
	while (true)
	{
		int res = config.validateConfig(configFile);
		if (res == -1)//에러
		{
			std::cout << "wow: error" << std::endl;
			return (-1);
		}
		if (res == 1)//모든 서버 검사를 마쳤을 때
			break;
	}
	
   // 1) 서버 키(포트)들 먼저 확인
    std::cout << "== servers(keys) ==\n";
    for (std::map<int, ServerConfig>::const_iterator it = config.servers.begin();
         it != config.servers.end(); ++it)
    {
        std::cout << "server key(port): " << it->first << "\n";
    }

    // 2) 특정 서버(8080) 내용 출력 (find로 안전하게)
    const int port = 990;
    std::map<int, ServerConfig>::const_iterator sit = config.servers.find(port);
    if (sit == config.servers.end())
    {
        std::cout << "server " << port << " not found\n";
        return (0);
    }
    const ServerConfig& srv = sit->second;

    std::cout << "\n== server " << port << " ==\n";
    std::cout << "client_max_body_size: " << srv.clientMaxBodySize << "\n";

    // 3) error_page(map<int,string>) 전부 출력
    std::cout << "\n[error_page]\n";
    for (std::map<int, std::string>::const_iterator eit = srv.errorPages.begin();
         eit != srv.errorPages.end(); ++eit)
    {
        std::cout << "  " << eit->first << " -> " << eit->second << "\n";
    }

    // 4) location(map<string, LocationConfig>) 전부 출력
    std::cout << "\n[locations]\n";
    for (std::map<std::string, LocationConfig>::const_iterator lit = srv.locations.begin();
         lit != srv.locations.end(); ++lit)
    {
        const std::string& prefix = lit->first;
        const LocationConfig& loc = lit->second;

        std::cout << "location " << prefix << "\n";
        std::cout << "  root: " << loc.getRoot() << "\n";
        std::cout << "  index: " << loc.getIndex() << "\n";
        std::cout << "  autoindex: " << (loc.getAutoIndex() ? "on" : "off") << "\n";
        std::cout << "  upload_dir: " << loc.getUploadDir() << "\n";
        std::cout << "  return: " << loc.getRedirectStatusCode() << " " << loc.getRedirectPath() << "\n";
        std::cout << "  cgi_ext: " << loc.getCgiExtension() << " " << loc.getCgiPath() << "\n";

        std::cout << "  methods:";
        const std::set<HttpMethod>& ms = loc.getMethods();
        for (std::set<HttpMethod>::const_iterator mit = ms.begin(); mit != ms.end(); ++mit)
            std::cout << " " << methodToStr(*mit);
        std::cout << "\n\n";
    }
	return (0);
}