#include "../../include/ServerConfig.hpp"

/**
 * @struct parseerrorPage
 * @brief 에러페이지 유효성 검사 및 값 세팅
 * 
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
 * @struct parselistenBody
 * @brief 서버 포트번호와 body size 유효성 검사 및 값 세팅
 * 
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
 * @struct parseServerDirective
 * @brief indent 1라인 진입함수
 * 
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
		locations[token[1]] = locConfig;
		return true;
	}
	
	return false;
}

/**
 * @struct isEndSequenceValid
 * @brief end 유효성 검사 함수
 * 
 * end 뒤에 server가 아닌 다른 문자열 올때 예외처리
 */
void ServerConfig::EndSequenceValid(std::ifstream &configFile)
{
	std::string nextLine;
	while (true)
	{
		std::streampos nextPos = configFile.tellg();
		if (!std::getline(configFile, nextLine))
		{
			statusMessage = "file end";
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
			return ;
		}
		statusMessage = "Config error: Invalid server block format";
		return ;
	}
}

ServerConfig::ServerConfig(std::ifstream &configFile, std::string configLine)
{
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
				EndSequenceValid(configFile);
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