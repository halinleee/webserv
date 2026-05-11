#include "RequestParser.hpp"
#include "HttpUtils.hpp"
#include "type.hpp"
#include <string>
#include <algorithm>

bool RequestParser::parseStartline(CharDq& buf)
{
	HttpUtils::consumeLeadingCRLF(buf);

	size_t crlf = HttpUtils::findCRLf(buf);
	if (crlf == HttpUtils::npos) return (false);
	std::string startLine = HttpUtils::extractLine(buf, crlf);

	if (!splitStartline(startLine, tmpReq))
	{
		state = REQ_ERROR;
		statusCode = STATUS_BAD_REQUEST;
		return (true);
	}

	if (!parseMethod(tmpReq.method) || !parseURI(tmpReq.target) || !parseVersion(tmpReq.version))
	{
		state = REQ_ERROR;
		return (true);
	}
	state = REQ_HEADERS;
	return (true);
}

bool RequestParser::splitStartline(const std::string& line, ReqLine& req)
{
	if (HttpUtils::hasCR(line) || line.size() > MAX_STARTLINE_LENGTH) return (false);

	size_t first = line.find(' ');
	size_t second = line.find(' ', first + 1);

	if (first == std::string::npos || second == std::string::npos) return (false);
	if (line.find(' ', second + 1) != std::string::npos) return (false);

	req.method = line.substr(0, first);
	req.target = line.substr(first + 1, second - first - 1);
	req.version = line.substr(second + 1);

	return (true);
}
bool RequestParser::parseMethod(const std::string& method)
{
	switch (method.size())
	{
		case 3:
			if (method == "GET") { parsed_req.method = METHOD_GET; return (true); }
			if (method == "PUT") { statusCode = STATUS_NOT_IMPLEMENTED; return (false); }
			break ;
		case 4:
			if (method == "POST") { parsed_req.method = METHOD_POST; return (true); }
			if (method == "HEAD") { statusCode = STATUS_NOT_IMPLEMENTED; return (false); }
			break ;
		case 5:
			if (method == "PATCH") { statusCode = STATUS_NOT_IMPLEMENTED; return (false); }
			if (method == "TRACE") { statusCode = STATUS_NOT_IMPLEMENTED; return (false); }
			break ;
		case 6:
			if (method == "DELETE") { parsed_req.method = METHOD_DELETE; return (true); }
			break ;
		case 7:
			if (method == "OPTIONS") { statusCode = STATUS_NOT_IMPLEMENTED; return (false); }
			if (method == "CONNECT") { statusCode = STATUS_NOT_IMPLEMENTED; return (false); }
			break ;
		parsed_req.method = METHOD_INVALID;
		statusCode = STATUS_BAD_REQUEST;
		return (false);
	}
}
bool RequestParser::parseURI(const std::string& target)
{
	if (target.empty() || target[0] != '/') { statusCode = STATUS_BAD_REQUEST; return (false); }
	if (std::count(target.begin(), target.end(), '?') > 1) { statusCode = STATUS_BAD_REQUEST; return (false); }
	if (target.size() > MAX_URI_LENGTH) { statusCode = STATUS_URI_LONG; return (false); }

	for(std::string::const_iterator it = target.begin(); it != target.end(); ++it)
	{
		unsigned char c = *it;
		if (c < 32 || c > 126) { statusCode = STATUS_BAD_REQUEST; return (false); }
	}
	if (!isValidPercentEncoding(target)) { statusCode = STATUS_BAD_REQUEST; return (false); }

	std::string path, query;
	splitURI(target, path, query);
	if (HttpUtils::hasConsecutiveSlashes(path) || HttpUtils::hasDotSegments(path)) { statusCode = STATUS_BAD_REQUEST; return (false); }
	
	std::string decoded;
	if (!percentDecode(path, decoded)) { statusCode = STATUS_BAD_REQUEST; return (false); }
	parsed_req.path = decoded;

	if (!query.empty()) parsed_req.query = query;
	return (true);
}
bool RequestParser::isValidPercentEncoding(const std::string& target)
{
	for(size_t i = 0; i < target.size(); ++i)
	{
		if (target[i] == '%')
		{
			if (i + 2 > target.size()) return (false);
			if (!HttpUtils::isHex(target[i + 1]) || !HttpUtils::isHex(target[i + 2])) return (false);
			i += 2;
		}
	}
	return (true);
}
void RequestParser::splitURI(const std::string& target, std::string& path, std::string& query)
{
	std::string trimmed;

	size_t fragment = target.find('#');
	if (fragment == std::string::npos) trimmed = target;
	else trimmed = target.substr(0, fragment);

	size_t pos = trimmed.find('?');
	if (pos == std::string::npos) path = trimmed;
	else
	{
		path = trimmed.substr(0, pos);
		query = trimmed.substr(pos + 1);
	}
}
bool RequestParser::percentDecode(const std::string& path, std::string& result)
{
	result.reserve(path.size());

	for(size_t i = 0; i < path.size(); ++i)
	{
		if (path[i] == '%')
		{
			int first = HttpUtils::hexToInt(path[i + 1]);
			int second = HttpUtils::hexToInt(path[i + 2]);
			unsigned char added = static_cast<unsigned char>(first << 4 | second);
			if (added == '\0') return (false);
			result += added;
			i += 2;
		}
		else result += path[i];
	}

	if (HttpUtils::hasConsecutiveSlashes(result)) return (false);
	return (true);
}
bool RequestParser::parseVersion(const std::string& version)
{
	if (version != "HTTP/1.0" && version != "HTTP/1.1") { statusCode = STATUS_BAD_REQUEST; return (false); }
	if (version == "HTTP/1.0") { statusCode = STATUS_HTTP_VERSION; return (false); }

	return (true);
}

bool RequestParser::parseHeaders(CharDq& buf)
{
	
}


RequestParser::RequestParser() : state(REQ_DONE), statusCode(STATUS_OK), hasBody(false) {}
RequestParser::~RequestParser() {}

void RequestParser::parse(CharDq& buf)
{
	switch (state)
	{
		case REQ_STARTLINE:
			if (!parseStartline(buf)) return ;
		case REQ_HEADERS:
			if (!parseHeaders(buf)) return ;
			// target URI 재구성
			// target URI 유효성 검사
			// 여기서 server block이나 location block도 결정한다고 함.....?
			state = hasBody ? REQ_BODY : REQ_DONE;
		case REQ_BODY:
			if (!parseBody(buf)) return ;
			state = REQ_DONE;
		case REQ_DONE:
			// 여기서 요청을 처리 or EPOLLOUT 키기, 요청 처리 후 Request struct도 clear
			clear(buf);
		case REQ_ERROR:
			; // 에러 응답 후 소켓 닫기
	}
}

ParseState RequestParser::getState() const { return (state); }