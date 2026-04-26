#include "../../include/Config.hpp"
/**
 * @struct parselistenBody
 * @brief 서버 포트번호와 body size 유효성 검사 및 값 세팅
 * 
 */
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


/**
 * @struct parseServerBlock
 * @brief server port 설정과 indent 1번 라인에 속해있는 라인 설정 함수(유효성 검사)
 * 
 * server에 포트번호가 맞게 왔는지 확인 뒤 map형태인 servers에 key값 등록
 * 
 * 들여쓰기(indent)가 맞게 적용되었는지 확인 뒤 indent 1번 라인을 검사 및 값 넣는 함수 적용(serverDirectiveValidate)
 * indent 1번 라인 예시 : client_max_body_size 10000000, error_page 404 /errors/404.html, location /files 등
 * 
 * server 포트로 열고 end로 잘 닫혔는지 검사함
 * 
 * return (1); //모든 서버 검사를 마쳤을때
 * 
 * return (0); //한개의 서버 검사를 마쳤을때
 * 
 * return (-1); //에러
 */
int Config::parseServerBlock(std::ifstream &configFile)
{
	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;
		break;
	}
	
	//서버 port가 없는지, 서버 전체 파싱이 끝났는지 확인
	if (configFile.eof() && servers.empty())
	{
		statusMessage = "Config error: config file is empty";
		return -1;
	}
	else if (configFile.eof() && configLine.empty())
		return 1;
	
	//server port 검사
	std::vector<std::string> serverToken = ftSplit(configLine, ' ');
	if (serverToken.empty())
	{
		statusMessage = "Config error: server token is empty";
		return -1;
	}
	if (serverToken[0] != "server" || !parseListen(serverToken, 65530))
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
	std::ifstream configFile("./config/webserv.conf");
	if (!configFile.is_open())
	{
		statusMessage = "Failed to open file";
		return ;
	}

	//.conf 확장자가 맞는지 확인하는 거 추가
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