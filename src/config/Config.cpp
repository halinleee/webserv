#include "../../include/Config.hpp"
/**
 * @brief `server <port>` 헤더의 포트 번호 형식을 검증한다.
 * @param token 공백 기준으로 분리된 토큰. 예: {"server", "8080"}
 * @param max 허용할 최대 포트 번호 (보통 65535)
 * @return 포트가 유효하면 true, 아니면 false
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
 * @brief config 파일에서 server 블록 1개를 파싱하여 서버 맵에 등록한다.
 * @details
 * - server 헤더(`server <port>`)를 읽고 포트를 검증한다.
 * - 이후 블록 본문은 ServerConfig 파서에 위임한다.
 * @param configFile 열린 config 파일 스트림
 * @retval  1 더 이상 읽을 server가 없어서 전체 파싱이 끝난 경우
 * @retval  0 server 1개를 정상 처리했고 다음 server를 계속 파싱해야 하는 경우
 * @retval -1 파싱 오류가 발생한 경우
 * @note 오류 상세는 statusMessage에 저장된다.
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
/**
 * @brief 기본 설정 파일(`./config/webserv.conf`)을 열고 전체 server 블록을 파싱한다.
 * @details 파일 오픈 실패 또는 파싱 실패 시 statusMessage에 오류 상태를 저장하고 종료한다.
 * @note 이 생성자는 예외를 던지지 않고 statusMessage로 상태를 전달한다.
 */
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

	std::ifstream configFile("./config/webserv.conf");
	if (!configFile.is_open())
	{
		statusMessage = "Failed to open file";
		return ;
	}

	//.conf 확장자가 맞는지 확인하는 거 추가
	//parseServerBlock의 반환값이 1이면 모든 서버 검증 완료. -1이면 에러. 나머지는 서버 한개 검증 완료.
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