#include "../../include/ServerConfig.hpp"

void ServerConfig::setPrefixes(void)
{
	for(std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
		it != locations.end(); ++it)
	{
		std::cout << it->first << std::endl;
		prefixes.push_back(it->first);
	}
	return ;
}
// string url : /files/*/babo /files/stella/steel.jpg
// prefixes :    /files -> /upload -> /redir
// 토큰단위로 비교하기(한글자씩 비교 안해도 됨)
LocationConfig ServerConfig::Matching(std::string url)
{
	std::vector<std::string> urlToken = ftSplit(url, '/');
	for (size_t i = 0; prefixes.size() > i; ++i)
	{
		if (prefixes[i] == urlToken[1])
	}
}


/**
 * @brief `error_page` 지시어를 검증하고 errorPages에 저장한다.
 * @details token 형식은 다음과 같아야 한다.
 * - error_page <status_code> <uri>
 * 예: {"error_page", "404", "/errors/404.html"}
 * @param token 공백 기준으로 분리된 지시어 토큰
 * @return 유효하면 true, 아니면 false
 */
bool ServerConfig::parseErrorPage(std::vector<std::string> &token)
{
	if (token.size() != 3)
		return false;

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return false;
	
	if (!isValidUriPath(token[2]))
		return false;
	errorPages[num] = token[2];
	return true;
}

/**
 * @brief body size 관련 지시어를 검증하고 값을 설정한다.
 * @details 현재는 `client_max_body_size`만 처리한다.
 * token 형식은 다음과 같아야 한다.
 * - client_max_body_size <size>
 * @param token 공백 기준으로 분리된 지시어 토큰
 * @param max 허용할 최대 크기
 * @return 유효하면 true, 아니면 false
 */
bool ServerConfig::parseBody(const std::vector<std::string> &token, size_t max)
{
	if (token.size() != 2)
		return false;

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return false;

	if (num == 0 || num > max)
		return false;

	if (token[0] == "client_max_body_size")
		clientMaxBodySize = (static_cast<size_t>(num));
	return true;
}

/**
 * @brief server 블록 내부(indent 1) 지시어를 파싱한다.
 * @details
 * 허용 지시어:
 * - client_max_body_size
 * - error_page
 * - location (location 블록 파싱은 LocationConfig에 위임)
 * @param token 공백 기준으로 분리된 지시어 토큰
 * @param configFile 열린 config 파일 스트림 (location 파싱 시 추가로 읽는다)
 * @return 유효한 지시어이고 처리에 성공하면 true, 아니면 false
 */
bool ServerConfig::parseServerDirective(std::vector<std::string> &token, std::ifstream &configFile)
{	
	if (token[0] == "client_max_body_size")
		return parseBody(token, 10000000);
	else if (token[0] == "error_page")
		return parseErrorPage(token);
	else if (token[0] == "location")
	{
		if (token.size() != 2 || !isValidPrefix(token[1]))
			return false;
		LocationConfig locConfig(configFile);
		if (!locConfig.isOk())
			return false;
		locations[token[1]] = locConfig; //prefix key값에 value대입
		return true;
	}
	
	return false;
}

/**
 * @brief `end` 이후에 올 수 있는 다음 블록을 검증하고 파싱 상태를 설정한다.
 * @details
 * `end` 다음에는 (빈 줄을 제외하고) 다음 server 헤더(`server <port>`)가 오거나,
 * 파일이 종료되어야 한다. 그 외의 문자열이 오면 config 형식 오류로 처리한다.
 *
 * 이 함수는 configFile의 위치를 필요 시 다음 server 헤더 시작 지점으로 되돌린다(seekg).
 *
 * @param configFile 열린 config 파일 스트림
 * @note 결과는 statusMessage에 다음 중 하나로 저장된다:
 * - "server end" : 다음 server 블록이 이어짐
 * - "file end"   : 파일 종료
 * - "Config error: Invalid server block format" : 형식 오류
 */
void ServerConfig::endSequenceValid(std::ifstream &configFile)
{
	std::string nextLine;
	while (true)
	{
		std::streampos nextPos = configFile.tellg();
		if (!std::getline(configFile, nextLine))
		{
			statusMessage = "file end";
			setPrefixes();
			return ;
		}
		if (isBlankLine(nextLine))
			continue;
		std::string tmp = nextLine;
		removeIndent(tmp, '\t');
		std::vector<std::string> nextToken = ftSplit(tmp, ' ');
		if (!nextToken.empty() && nextToken[0] == "server")
		{
			configFile.clear();
			configFile.seekg(nextPos);
			statusMessage = "server end";
			setPrefixes();
			return ;
		}
		statusMessage = "Config error: Invalid server block format";
		return ;
	}
}

/**
 * @brief server 블록 본문을 파싱하여 ServerConfig를 구성한다.
 * @details
 * 이 생성자는 server 헤더(server <port>) 다음 라인부터 읽기 시작하여,
 * 들여쓰기 규칙에 따라 server 블록을 구성하는 지시어들을 처리한다.
 *
 * 처리 규칙(현재 구현 기준):
 * - indent 1: server 지시어(client_max_body_size, error_page, location)
 * - indent 0: "end"를 만나면 server 블록 종료
 * - indent > 1 또는 indent == -1: 들여쓰기 오류로 실패
 *
 * @param configFile 열린 config 파일 스트림 (현재 server 블록의 본문을 읽는다)
 * @param configLine 호출 시점의 라인 버퍼(입력/출력 용도)
 * @note 파싱 결과/오류는 statusMessage에 저장되며, 예외는 던지지 않는다.
 */
ServerConfig::ServerConfig(std::ifstream &configFile, std::string configLine)
{
	clientMaxBodySize = 1000000;

	statusMessage = "Default Error";
	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;

		int indent = countIndent(configLine);
		if (indent > 1 || indent == -1)
		{
			statusMessage = "Config error: indent error\nerror line: " + configLine;
			return ;
		}

		if (indent == 1)
		{
			removeIndent(configLine, '\t');
			std::vector<std::string> DirectiveToken = ftSplit(configLine, ' ');
			if (DirectiveToken.empty())
			{
				statusMessage = "Config error: Directive token is empty";
				return ;
			}
			
			if (!parseServerDirective(DirectiveToken, configFile))
			{
				statusMessage = "Config error: Invalid server block format\nerror line: " + configLine;
				return ;
			}
		}

		if (indent == 0)
		{
			if (configLine == "end")
			{
				if (locations.empty())
				{
					statusMessage = "Config error: location is not defined";
					return ;
				}
				endSequenceValid(configFile);
				return ;
			}
			else
			{
				statusMessage = "Config error: Invalid server block format";
				return ;
			}
		}
	}
	statusMessage = "Config error: Invalid server block format";
	return ;
}