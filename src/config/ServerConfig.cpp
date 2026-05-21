#include "../../include/ServerConfig.hpp" // ServerConfig 정의
#include "../../include/Util.hpp"         // toInt/isValidUriPath/isValidPrefix/isBlankLine/removeIndent/ftSplit/countIndent

#include <fstream> // std::ifstream, std::getline, std::streampos 사용(구현부)

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

    size_t highScore = 0;
    bool match = false;

    for (size_t i = 0; i < prefixes.size(); ++i)
    {
        const std::string& prefix = prefixes[i];
        size_t prefixLen = prefix.size();
        size_t urlLen = url.size();

        if (urlLen < prefixLen)
            continue;
		
        if (url.compare(0, prefixLen, prefix) != 0)
            continue;

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
		LocationConfig locConfig(configFile, token[1]);
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