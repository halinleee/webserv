#include "./include/Config.hpp"

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
	std::map<int, ServerConfig>::const_iterator it;
	std::map<int, std::string>::const_iterator eit;
	it = config.servers.find(8080);
	eit = it->second.errorPages.find(404);
	std::cout << it->second.clientMaxBodySize << std::endl;
	std::cout << eit->second << std::endl;
	
	return (0);
}