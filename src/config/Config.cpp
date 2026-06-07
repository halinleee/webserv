#include "Config.hpp"
#include "Util.hpp"

#include <fstream>

#define PORT_MAX 65535

bool Config::isValidListen(const std::vector<std::string>& token)
{
	if (token.size() != 2) return false;

	size_t num = 0;
	if (!toInt(token[1], num)) return false;

	if (num == 0 || num > PORT_MAX) return false;
	return true;
}

parseStatus Config::parseServerBlock(std::ifstream &configFile)
{
	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine)) continue;
		break;
	}
	
	//server port가 없는지, 서버 전체 파싱이 끝났는지 확인
	if (configFile.eof() && servers.empty())
	{ 
		statusMessage = "Config error: config file is empty"; 
		return PARSE_ERROR; 
	}
	else if (configFile.eof() && configLine.empty()) 
		return PARSE_SERVER_END;
	
	//server port 검사
	if (countIndent(configLine) != 0)
	{
		statusMessage = "Config error: server port line indent error";
		return PARSE_ERROR;
	}

	std::vector<std::string> serverToken = ftSplit(configLine, ' ');
	if (serverToken.empty())
	{ 
		statusMessage = "Config error: server token is empty"; 
		return PARSE_ERROR; 
	}

	if (serverToken[0] != "server" || !isValidListen(serverToken))
	{ 
		statusMessage = "Config error: server or port error"; 
		return PARSE_ERROR; 
	}

	size_t tmp = 0;
	if (!toInt(serverToken[1], tmp))
	{ 
		statusMessage = "Config error: port config error";
		return PARSE_ERROR; 
	}
	in_port_t key = static_cast<in_port_t>(tmp);

	//server 파싱 시작
	ServerConfig server;
	parseStatus result = server.parseServerConfigBlock(configFile);
	statusMessage = server.getStatusMessage();

	if (result == PARSE_ERROR)
		return PARSE_ERROR;
	if (result == PARSE_FILE_END)
	{ 
		servers[key] = server;
		return PARSE_FILE_END; 
	}
	else if (result == PARSE_SERVER_END)
	{ 
		servers[key] = server;
		return PARSE_SERVER_END; 
	}
	else
		return PARSE_ERROR;
}

bool Config::parseConfig()
{
	/* todo
	std::string configPath;
    if (argc == 2)
        configPath = argv[1];
    else
        configPath = "./config/webserv.conf"; // default
    std::ifstream configFile(configPath.c_str());
    if (!configFile.is_open())
    {
        std::cerr << "Failed to open config file: " << configPath << std::endl;
        return 1;
    }
	*/

	//main 연결할때까지 인자값 받았다 치고 임시로 이 코드 돌려유
	std::ifstream configFile("./webserv.conf");
	if (!configFile.is_open())
	{
		statusMessage = "Failed to open file";
		return false;
	}

	while (true)
	{
		int res = parseServerBlock(configFile);
		if (res == PARSE_ERROR)
			return false;
		if (res == PARSE_FILE_END)
			break;
	}
	statusMessage = "ok";
	return true;
}