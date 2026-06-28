#include "CgiParser.hpp"

bool CgiParser::parseStatusLine(const std::string& lines, Response& response)
{
	std::vector<std::string> statusLine = ftSplit(lines, ' ');
	if (statusLine.empty() || statusLine.size() != 3)
		return false;
	if (statusLine[0] == "HTTP/1.1")
	{
		size_t num = 0;
		if (!toInt(statusLine[1], num) || num < 100 || num > 599)
			return false;
		response.statusCode = static_cast<Status>(num);
		response.statusText = HttpUtils::getStatusText(response.statusCode);
	}
	return true;
}

bool CgiParser::splitHeaderBody(const std::string& stdout, Response& response, std::string& headers)
{
	size_t pos = stdout.find("\r\n\r\n");
	size_t sepLen = 4;

	if (pos == std::string::npos)
	{
		pos = stdout.find("\n\n");
		sepLen = 2;
		if (pos == std::string::npos)
			return false;
	}
	headers =  stdout.substr(0, pos);
	response.body = stdout.substr(pos + sepLen);
	return true;
}

bool CgiParser::cgiParseKeyValue(const std::string& line, std::string& key, std::string& value)
{
	size_t colon = line.find(':');
	if (colon == std::string::npos || colon == 0) return false;

	key = line.substr(0, colon);

	for(size_t i = 0; i < key.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(key[i]);
		if (!HttpUtils::isTchar(c)) return false;
		key[i] = std::tolower(c);
	}

	value = line.substr(colon + 1);

	size_t start = value.find_first_not_of(" \t");
	if (start != std::string::npos) value = value.substr(start);
	else value.clear();

	size_t end = value.find_last_not_of(" \t");
	if (end != std::string::npos) value = value.substr(0, end + 1);
	else value.clear();

	for(size_t i = 0; i < value.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(value[i]);
		if (!HttpUtils::isVcharSpTab(c)) return false;
	}
	return true;
}

Response CgiParser::parseCgiOutput(const std::string& cgiOutput)
{
	Response response;
	std::string headers;
	if (!splitHeaderBody(cgiOutput, response, headers))
		return Response(STATUS_BAD_GATEWAY);

	std::vector<std::string> lines = ftSplit(headers, '\n');
	for (int i = 0; i < lines.size(); i++)
	{
		if (!lines[i].empty() && lines[i][lines[i].size() - 1] == '\r')
			lines[i].erase(lines[i].size() - 1);
	}

	if (!parseStatusLine(lines[0], response))
		return Response(STATUS_BAD_GATEWAY);
	return response;
}