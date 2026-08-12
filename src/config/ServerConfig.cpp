#include "ServerConfig.hpp"
#include "ConfigParseUtils.hpp"
#include "type.hpp"

#include <fstream>
#include <arpa/inet.h>
#include <algorithm>

void ServerConfig::setPrefixes(void)
{
	prefixes.clear();
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
            matchLocation = locations.find(prefix)->second;
            matchedPrefix = prefix;
            match = true;
        }
    }

    if (!match)
    {
        std::map<std::string, LocationConfig>::iterator it = locations.find("/");
        if (it == locations.end())
            return false;
        matchLocation = it->second;
        matchedPrefix = it->first;
    }
    return true;
}

bool ServerConfig::parseTimeOut(std::vector<std::string>& token)
{
	if (token.size() != 2)
		return false;

	size_t num = 0;

	if (!toInt(token[1], num))
		return false;

	if (num == 0 || num > TIME_OUT_MAX)
		return false;

	if (token[0] == "connection_timeout")
		timeConfig.connectionTimeOut = static_cast<std::time_t>(num);
	else if (token[0] == "read_timeout")
		timeConfig.readTimeout = static_cast<std::time_t>(num);
	else if (token[0] == "write_timeout")
		timeConfig.writeTimeout = static_cast<std::time_t>(num);
	else if (token[0] == "keep_alive_timeout")
		timeConfig.keepAliveTimeout = static_cast<std::time_t>(num);
	else if (token[0] == "cgi_timeout")
		timeConfig.cgiTimeout = static_cast<std::time_t>(num);


	return true;
}

bool isValidErrorCode(size_t code)
{
	switch (code)
	{
		case STATUS_BAD_REQUEST:
		case STATUS_FORBIDDEN:
		case STATUS_NOT_FOUND:
		case STATUS_METHOD_NOT_ALLOWED:
		case STATUS_REQUEST_TIMEOUT:
		case STATUS_PAYLOAD_TOO_LARGE:
		case STATUS_URI_LONG:
		case STATUS_HEADER_TOO_LARGE:
		case STATUS_INTERNAL_SERVER_ERROR:
		case STATUS_NOT_IMPLEMENTED:
		case STATUS_BAD_GATEWAY:
		case STATUS_SERVICE_UNAVAILABLE:
		case STATUS_GATEWAY_TIMEOUT:
		case STATUS_HTTP_VERSION:
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

	if (!isValidFileSystemPath(token[2]))
		return false;

	if(!isValidErrorCode(num))
		return true;

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

bool ServerConfig::parseListen(std::vector<std::string> &token)
{
	if (token.size() != 2)
		return false;

	size_t maxIpSize = token[1].size();

	if (maxIpSize > 15)
		return false;

	if (std::count(token[1].begin(), token[1].end(), '.') != 3)
		return false;

	std::vector<std::string> octetToken = ftSplit(token[1], '.');
	if (octetToken.size() != 4)
		return false;

	size_t octet1, octet2, octet3, octet4;

	if (!toInt(octetToken[0], octet1) || !toInt(octetToken[1], octet2) || !toInt(octetToken[2], octet3) || !toInt(octetToken[3], octet4))
		return false;

	if (octet1 < 256 && octet2 < 256 && octet3< 256 && octet4 < 256)
	{
		listen = htonl((octet1 << 24) | (octet2 << 16) | (octet3 << 8) | octet4);
		return true;
	}
	return false;
}

bool ServerConfig::parseServerDirective(std::vector<std::string> &token, std::ifstream &configFile)
{
	if (token[0] == "client_max_body_size")
		return parseBody(token);
	else if (token[0] == "listen")
		return parseListen(token);
	else if (token[0] == "error_page")
		return parseErrorPage(token);
	else if (token[0] == "connection_timeout" || token[0] == "read_timeout" || token[0] == "write_timeout"
			|| token[0] == "keep_alive_timeout" || token[0] == "cgi_timeout")
		return parseTimeOut(token);
	else if (token[0] == "location")
	{
		if (token.size() != 2 || !isValidNormalizePath(token[1]))
			return false;

		LocationConfig locConfig;
		if (!locConfig.parseLocationBlock(configFile, token[1]))
			return false;

		bool hasRoot = !locConfig.getRoot().empty();
		bool hasReturn = !locConfig.getRedirectPath().empty();
		bool hasAlias = !locConfig.getAlias().empty();

		if (!hasRoot && !hasReturn && !hasAlias)
			return false;
		if (hasRoot && hasAlias)
			return false;

		locations[token[1]] = locConfig;
		return true;
	}

	return false;
}

parseStatus ServerConfig::parseServerConfigBlock(std::ifstream &configFile)
{
	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;

		int indent = countIndent(configLine);
		if (indent > 1 || indent == -1)
		{
			statusMessage = "Config error: indent error";
			return PARSE_ERROR;
		}

		if (indent == 1)
		{
			removeIndent(configLine, '\t');
			std::vector<std::string> directiveToken = ftSplit(configLine, ' ');
			if (directiveToken.empty())
			{
				statusMessage = "Config error: Directive token is empty";
				return PARSE_ERROR;
			}

			if (!parseServerDirective(directiveToken, configFile))
			{
				statusMessage = "Config error: Invalid server block format";
				return PARSE_ERROR;
			}
		}

		if (indent == 0)
		{
			if (configLine == "end")
			{
				if (locations.empty())
				{
					statusMessage = "Config error: location is not defined";
					return PARSE_ERROR;
				}
				setPrefixes();
				return PARSE_SERVER_END;

			}
			else
			{
				statusMessage = "Config error: Invalid server block format";
				return PARSE_ERROR;
			}
		}
	}
	statusMessage = "Config error: Invalid server block format";
	return PARSE_ERROR;
}