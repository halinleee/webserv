#include "ServerConfig.hpp"
#include "Util.hpp"
#include "type.hpp"
#include <fstream>

#define BODY_SIZE_MAX (5 * 1024 * 1024) //5242880 bytes
#define TIME_OUT_MAX 180

void ServerConfig::setPrefixes(void)
{
	for(std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
		it != locations.end(); ++it)
	{
		prefixes.push_back(it->first);
	}
	return ;
}

bool ServerConfig::matching(const std::string& url)
{
    if (url.empty() || url[0] != '/')
        return false;

    size_t highScore = 0; //가장 긴 prefix 길이 저장하는 변수
    bool match = false; //매칭이 한번이라도 성공했는지에 대한 변수

    for (size_t i = 0; i < prefixes.size(); ++i)
    {
        const std::string& prefix = prefixes[i];
        size_t prefixLen = prefix.size();
        size_t urlLen = url.size();

        if (urlLen < prefixLen) //url이 prefix보다 짧으면 스킵
            continue;
		
        if (url.compare(0, prefixLen, prefix) != 0) //url의 앞부분이 prefix와 정확히 같지 않으면 스킵
            continue;
		/*
		경계 체크
		prefix가 '/'로 끝나지 않는 경우(ex: "/bin")에는 "/bin123"과 같은 케이스를 매칭하면 안됨
		prefix 마지막이 '/'가 아니고 url이 prefix보다 길어서 뒤에 경로가 더 존재하며
		url[prefix] 길이 뒤에 문자가 '/'가 아니면 "/bin"가 "/bin123"처럼 잘못 매칭되는 상황이므로 스킵
		ex)
			url: /cgi_bin/cgi/bin123/abc/files [x]
			url: /cgi_bin/cgi/bin/abc/files    [o]
	 		location /cgi_bin/cgi/bin
		*/
        if (prefix[prefixLen - 1] != '/' && urlLen > prefixLen && url[prefixLen] != '/')
            continue;

        if (prefixLen > highScore)
        {
            highScore = prefixLen;
            matchLocation = locations[prefix];
            match = true;
        }
    }

    if (!match)
    {
        std::map<std::string, LocationConfig>::iterator it = locations.find("/");
        if (it == locations.end())
            return false;
        matchLocation = it->second;
    }
    return true;
}

bool ServerConfig::parseKeepAlive(std::vector<std::string>& token)
{
	if (token.size() != 2)
		return false;

	if (token[0] != "keepalive_timeout")
		return false;

	size_t num = 0;

	if (!toInt(token[1], num))
		return false;

	if (num == 0 || num > TIME_OUT_MAX)
		return false;

	keepAliveTimeout = static_cast<size_t>(num);

	return true;
}

bool isValidErrorCode(size_t code)
{
	switch (code)
	{
		case STATUS_BAD_REQUEST:
		case STATUS_URI_LONG:
		case STATUS_NOT_IMPLEMENTED:
		case STATUS_HTTP_VERSION:
		case STATUS_HEADER_TOO_LARGE:
		case  STATUS_PAYLOAD_TOO_LARGE:
			return true;
		default:
			return false;
	}
}

bool ServerConfig::parseErrorPage(std::vector<std::string> &token)
{
	if (token.size() != 3)
		return false;

	size_t num = 0;
	if (!toInt(token[1], num))
		return false;
	
	if (!isValidNormalizePath(token[2]))
		return false;
	
	if(!isValidErrorCode(num))
		return false;

	errorPages[num] = token[2];
	return true;
}

bool ServerConfig::parseBody(const std::vector<std::string> &token)
{
	if (token.size() != 2)
		return false;

	if (token[0] != "client_max_body_size")
		return false;

	size_t num = 0;

	if (!toInt(token[1], num))
		return false;

	if (num == 0 || num > BODY_SIZE_MAX)
		return false;

	clientMaxBodySize = num;

	return true;
}


bool ServerConfig::parseServerDirective(std::vector<std::string> &token, std::ifstream &configFile)
{	
	if (token[0] == "client_max_body_size")
		return parseBody(token);
	else if (token[0] == "error_page")
		return parseErrorPage(token);
	else if (token[0] == "keepalive_timeout")
		return parseKeepAlive(token);
	else if (token[0] == "location")
	{
		if (token.size() != 2 || !isValidNormalizePath(token[1]))
			return false;
		LocationConfig locConfig(configFile);
		if (!locConfig.isOk())
			return false;
		locations[token[1]] = locConfig; //prefix key값에 value 대입
		return true;
	}
	
	return false;
}


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


ServerConfig::ServerConfig(std::ifstream &configFile)
{
	clientMaxBodySize = 1000000;
	statusMessage = "Default Error";
	keepAliveTimeout = 75;

	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;

		int indent = countIndent(configLine);
		if (indent > 1 || indent == -1)
			{ statusMessage = "Config error: indent error\nerror line: " + configLine; return ; }

		if (indent == 1)
		{
			removeIndent(configLine, '\t');
			std::vector<std::string> directiveToken = ftSplit(configLine, ' ');
			if (directiveToken.empty())
			{ statusMessage = "Config error: Directive token is empty"; return ; }

			if (!parseServerDirective(directiveToken, configFile))
			{ statusMessage = "Config error: Invalid server block format\nerror line: " + configLine; return ; }
		}

		if (indent == 0)
		{
			if (configLine == "end")
			{
				if (locations.empty())
				{ statusMessage = "Config error: location is not defined"; return ; }
				endSequenceValid(configFile); return ;
			}
			else
			{ statusMessage = "Config error: Invalid server block format"; return ;}
		}
	}
	statusMessage = "Config error: Invalid server block format";
	return ;
}