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

	for (std::map<int, ServerConfig>::const_iterator sit = config.servers.begin();
         sit != config.servers.end(); ++sit)
    {
        const int port = sit->first;
        const ServerConfig& srv = sit->second;

        std::cout << "=== server " << port << " ===\n";
        std::cout << "client_max_body_size: " << srv.clientMaxBodySize << "\n";

        std::cout << "[error_page]\n";
        for (std::map<int, std::string>::const_iterator eit = srv.errorPages.begin();
             eit != srv.errorPages.end(); ++eit)
            std::cout << "  " << eit->first << " -> " << eit->second << "\n";

        std::cout << "[locations]\n";
        for (std::map<std::string, LocationConfig>::const_iterator lit = srv.locations.begin();
             lit != srv.locations.end(); ++lit)
        {
            const std::string& prefix = lit->first;
            const LocationConfig& loc = lit->second;

            std::cout << "  location " << prefix << "\n";
            std::cout << "    root: " << loc.getRoot() << "\n";
            std::cout << "    index: " << loc.getIndex() << "\n";
            std::cout << "    autoindex: " << (loc.getAutoIndex() ? "on" : "off") << "\n";
            std::cout << "    upload_dir: " << loc.getUploadDir() << "\n";
            std::cout << "    return: " << loc.getRedirectStatusCode()
                      << " " << loc.getRedirectPath() << "\n";
            std::cout << "    cgi_ext: " << loc.getCgiExtension()
                      << " " << loc.getCgiPath() << "\n";

            std::cout << "    methods:";
            const std::set<HttpMethod>& ms = loc.getMethods();
            for (std::set<HttpMethod>::const_iterator mit = ms.begin(); mit != ms.end(); ++mit)
                std::cout << " " << methodToStr(*mit);
            std::cout << "\n";
        }
	}
	return (0);
}