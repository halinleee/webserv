#include "../../include/Config.hpp" // Config 선언
#include "../../include/Util.hpp"   // isBlankLine/ftSplit/toInt 등

#include <fstream> // std::ifstream (구현에서 파일 열기)

std::string basePath = "";

bool Config::parseListen(const std::vector<std::string>& token, size_t max)
{
	if (token.size() != 2)
		return false;

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return false;

	if (num == 0 || num > max)
		return false;
	return true;
}

int Config::parseServerBlock(std::ifstream &configFile)
{
	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;
		break;
	}
	
	//server port가 없는지, 서버 전체 파싱이 끝났는지 확인
	if (configFile.eof() && servers.empty())
	{
		statusMessage = "Config error: config file is empty";
		return -1;
	}
	else if (configFile.eof() && configLine.empty()) return 1;
	
	//server port 검사
	std::vector<std::string> serverToken = ftSplit(configLine, ' ');
	if (serverToken.empty())
	{
		statusMessage = "Config error: server token is empty";
		return -1;
	}
	if (serverToken[0] != "server" || !parseListen(serverToken, 65535))
	{
		statusMessage = "Config error: server or port error\nerror line: " + configLine;
		return -1;
	}
	unsigned int key = 0;
	if (!toInt(serverToken[1], key))
	{
		statusMessage = "Config error: port config error\nerror line: " + configLine;
		return -1;
	}
	
	ServerConfig server(configFile, configLine);
	statusMessage = server.getStatusMessage();
	if (statusMessage == "file end")
	{
		servers[key] = server;
		return 1;
	}
	else if (statusMessage == "server end")
	{
		servers[key] = server;
		return 0;
	}
	else
		return -1;
}

Config::Config()
{
	// std::string configPath;
    // if (argc == 2)
    //     configPath = argv[1];
    // else
    //     configPath = "./config/webserv.conf"; // default
    // std::ifstream configFile(configPath.c_str());
    // if (!configFile.is_open())
    // {
    //     std::cerr << "Failed to open config file: " << configPath << std::endl;
    //     return 1;
    // }

	//main 연결할때까지 인자값 받았다 치고 임시로 이 코드 돌려유
	std::ifstream configFile("./webserv.conf");
	if (!configFile.is_open())
	{
		statusMessage = "Failed to open file";
		return ;
	}

	//main 연결되면 문자열을 av로 변환
	basePath = "/home/gajeon/webserv";
	while (true)
	{
		int res = parseServerBlock(configFile);
		if (res == -1)
			return ;
		if (res == 1)
			break;
	}
	statusMessage = "ok";
	return ;
}