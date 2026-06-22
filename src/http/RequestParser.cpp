#include "RequestParser.hpp"
#include "HttpUtils.hpp"
#include "type.hpp"
#include <string>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdlib>

bool RequestParser::parseStartline(CharDq& buf)
{
	HttpUtils::consumeLeadingCRLF(buf);

	size_t crlf = HttpUtils::findCRLF(buf);
	if (crlf == HttpUtils::npos) return false;
	std::string startLine = HttpUtils::extractLine(buf, crlf, 2);

	if (!splitStartline(startLine, tmpReqLine)) { statusCode = STATUS_BAD_REQUEST; return true;	}

	if (!parseMethod(tmpReqLine.method) || !parseURI(tmpReqLine.target) || !parseVersion(tmpReqLine.version))
		return true;
	return true;
}

bool RequestParser::splitStartline(const std::string& line, ReqLine& req)
{
	if (HttpUtils::hasCR(line) || line.size() > MAX_STARTLINE_LENGTH) return false;

	size_t first = line.find(' ');
	size_t second = line.find(' ', first + 1);

	if (first == std::string::npos || second == std::string::npos) return false;
	if (line.find(' ', second + 1) != std::string::npos) return false;

	req.method = line.substr(0, first);
	req.target = line.substr(first + 1, second - first - 1);
	req.version = line.substr(second + 1);

	return true;
}
bool RequestParser::parseMethod(const std::string& method)
{
	if (method == "GET") { parsedReq.method = METHOD_GET; return true; }
	if (method == "POST") { parsedReq.method = METHOD_POST; return true; }
	if (method == "DELETE") { parsedReq.method = METHOD_DELETE; return true; }
	if (method == "PUT" || method == "HEAD" || method == "PATCH" || 
		method == "TRACE" || method == "OPTIONS" || method == "CONNECT")
		{ statusCode = STATUS_NOT_IMPLEMENTED; parsedReq.method = METHOD_INVALID; return false; }
	statusCode = STATUS_BAD_REQUEST;
	parsedReq.method = METHOD_INVALID;
	return false;
}
bool RequestParser::parseURI(const std::string& target)
{
	if (target.empty() || target[0] != '/') { statusCode = STATUS_BAD_REQUEST; return false; }
	if (std::count(target.begin(), target.end(), '?') > 1) { statusCode = STATUS_BAD_REQUEST; return false; }
	if (target.size() > MAX_URI_LENGTH) { statusCode = STATUS_URI_LONG; return false; }

	for(std::string::const_iterator it = target.begin(); it != target.end(); ++it)
	{
		unsigned char c = *it;
		if (c < 32 || c > 126) { statusCode = STATUS_BAD_REQUEST; return false; }
	}
	if (!isValidPercentEncoding(target)) { statusCode = STATUS_BAD_REQUEST; return false; }

	std::string path, query, decoded;
	if (!splitURI(target, path, query)) { statusCode = STATUS_BAD_REQUEST; return false;}
	if (!percentDecode(path, decoded)) { statusCode = STATUS_BAD_REQUEST; return false; }
	if (HttpUtils::hasConsecutiveSlashes(decoded) || HttpUtils::hasDotSegments(decoded)) 
		{ statusCode = STATUS_BAD_REQUEST; return false; }

	parsedReq.path = decoded;
	if (!query.empty()) parsedReq.query = query; // query decoding 및 파싱은 cgi 책임.
	return true;
}
bool RequestParser::isValidPercentEncoding(const std::string& target)
{
	for(size_t i = 0; i < target.size(); ++i)
	{
		if (target[i] == '%')
		{
			if (i + 2 >= target.size()) return false;
			if (!HttpUtils::isHex(target[i + 1]) || !HttpUtils::isHex(target[i + 2])) return false;
			i += 2;
		}
	}
	return true;
}
bool RequestParser::splitURI(const std::string& target, std::string& path, std::string& query)
{
	size_t fragment = target.find('#');
	if (fragment != std::string::npos) return false;

	size_t pos = target.find('?');
	if (pos == std::string::npos) path = target;
	else
	{
		path = target.substr(0, pos);
		query = target.substr(pos + 1);
	}
	return true;
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
			if (added == '\0') return false;
			result += added;
			i += 2;
		}
		else result += path[i];
	}

	return true;
}
bool RequestParser::parseVersion(const std::string& version)
{
	if (version != "HTTP/1.0" && version != "HTTP/1.1")
		{ statusCode = STATUS_BAD_REQUEST; return false; }
	if (version != "HTTP/1.1") { statusCode = STATUS_HTTP_VERSION; return false; }

	return true;
}

bool RequestParser::parseHeaders(CharDq& buf)
{
	if (buf.size() >= 2 && buf[0] == '\r' && buf[1] == '\n')
	{
		buf.erase(buf.begin(), buf.begin() + 2);
		validateHeaders();
		return true;
	}

	size_t crlfcrlf = HttpUtils::findCRLFCRLF(buf);
	if (crlfcrlf == HttpUtils::npos) return false;
	if (crlfcrlf > MAX_HEADER_SECTION_LENGTH)
		{ statusCode = STATUS_HEADER_TOO_LARGE; return true; }

	CharDq headers(buf.begin(), buf.begin() + crlfcrlf + 2);
	buf.erase(buf.begin(), buf.begin() + crlfcrlf + 4);

	while (!headers.empty())
	{
		size_t end = HttpUtils::findCRLF(headers);
		if (end == HttpUtils::npos) { statusCode = STATUS_BAD_REQUEST; return true; }

		std::string line = HttpUtils::extractLine(headers, end, 2);
		if (line.size() > MAX_HEADER_LINE_LENGTH)
			{ statusCode = STATUS_HEADER_TOO_LARGE; return true; }
		if (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
			{ statusCode = STATUS_BAD_REQUEST; return true; }

		std::string key, value;
		if (!parseKeyValue(line, key, value)) { statusCode = STATUS_BAD_REQUEST; return true; }
		tmpHeaders[key].push_back(value);
	}
	
	if (!validateHeaders()) return true;
	transferHeaders();
	return true;
}
bool RequestParser::parseKeyValue(const std::string& line, std::string& key, std::string& value)
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
bool RequestParser::validateHeaders()
{
	if (tmpHeaders.find("host") == tmpHeaders.end() || tmpHeaders["host"].size() != 1)
		{ statusCode = STATUS_BAD_REQUEST; return false; }
	if (!parseHost(tmpHeaders["host"][0])) { statusCode = STATUS_BAD_REQUEST; return false; }

	bool hasCL = tmpHeaders["content-length"].size() > 0;
	bool hasTE = tmpHeaders["transfer-encoding"].size() > 0;
	if (hasCL && hasTE) { statusCode = STATUS_BAD_REQUEST; return false; }
	if (hasCL && !validateContentLength(tmpHeaders["content-length"])) return false;
	if (hasTE && !validateTransferEncoding(tmpHeaders["transfer-encoding"])) return false;

	return true;
}
bool RequestParser::parseHost(const std::string& raw)
{
	size_t colon = raw.find(':');
	if (colon == std::string::npos) 
	{
		if (raw.empty()) { statusCode = STATUS_BAD_REQUEST; return false; }
		parsedReq.host = raw; 
		parsedReq.port = 80; 
		return true;
	}
	if (raw.find(':', colon + 1) != std::string::npos) { statusCode = STATUS_BAD_REQUEST; return false; }
	
	std::string host = raw.substr(0, colon);
	std::string strPort = raw.substr(colon + 1);
	if (host.empty() || strPort.empty()) { statusCode = STATUS_BAD_REQUEST; return false; }

	for(size_t i = 0; i < strPort.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(strPort[i])))
			{ statusCode = STATUS_BAD_REQUEST; return false; }
	}

	char* end = NULL;
	long numPort = std::strtol(strPort.c_str(), &end, 10);
	if (*end != '\0' || numPort < 0 || numPort > 65535)
		{ statusCode = STATUS_BAD_REQUEST; return false; }

	parsedReq.host = host;
	parsedReq.port = static_cast<in_port_t>(numPort);
	return true;
}
bool RequestParser::validateContentLength(const strVec& cl)
{
	strVec values;
	std::string tmpStr;
	
	for(size_t i = 0; i < cl.size(); ++i)
	{
		std::stringstream ss(cl[i]);
		while (std::getline(ss, tmpStr, ','))
		{
			size_t s = tmpStr.find_first_not_of(" \t");
			size_t e = tmpStr.find_last_not_of(" \t");
			if (s == std::string::npos) { statusCode = STATUS_BAD_REQUEST; return false; }
			std::string val = tmpStr.substr(s, e - s + 1);
			if (val.empty()) { statusCode = STATUS_BAD_REQUEST; return false; }
			for(size_t j = 0; j < val.size(); ++j)
			{
				if (!std::isdigit(static_cast<unsigned char>(val[j])))
					{ statusCode = STATUS_BAD_REQUEST; return false; }
			}
			values.push_back(val);
		}
	}
	if (values.empty()) { statusCode = STATUS_BAD_REQUEST; return false; }

	for(size_t i = 1; i < values.size(); ++i)
	{
		if (values[i] != values[0]) { statusCode = STATUS_BAD_REQUEST; return false; }
	}

	char* end = NULL;
	unsigned long n = std::strtoul(values[0].c_str(), &end, 10);
	if (*end != '\0') { statusCode = STATUS_BAD_REQUEST; return false; }
	if (n > MAX_CLIENT_BODY_LENGTH)													// TODO
		{ statusCode = STATUS_PAYLOAD_TOO_LARGE; return false; }
	parsedReq.contentLength = static_cast<long long>(n);
	return true;
}
bool RequestParser::validateTransferEncoding(const strVec& te)
{
	strVec values;
	std::string tmpStr;

	for(size_t i = 0; i < te.size(); ++i)
	{
		std::stringstream ss(te[i]);
		while (std::getline(ss, tmpStr, ','))
		{
			size_t s = tmpStr.find_first_not_of(" \t");
			size_t e = tmpStr.find_last_not_of(" \t");
			if (s == std::string::npos) { statusCode = STATUS_BAD_REQUEST; return false; }
			std::string val = tmpStr.substr(s, e - s + 1);
			if (val.empty()) { statusCode = STATUS_BAD_REQUEST; return false; }
			for(size_t j = 0; j < val.size(); ++j)
				val[j] = std::tolower(static_cast<unsigned char>(val[j]));
			values.push_back(val);
		}
	}
	if (values.empty()) { statusCode = STATUS_BAD_REQUEST; return false; }

	for (size_t i = 0; i < values.size(); ++i)
	{
		if (values[i] != "chunked") { statusCode = STATUS_NOT_IMPLEMENTED; return false; }
	}
	if (values.size() > 1) { statusCode = STATUS_BAD_REQUEST; return false; }
	parsedReq.isChunked = true;
	return true;
}
void RequestParser::transferHeaders()
{
	for (std::map<std::string, strVec>::iterator it = tmpHeaders.begin(); it != tmpHeaders.end(); ++it)
	{
		const std::string& key = it->first;
		const strVec& values = it->second;

		if (values.empty()) continue;

		const std::string sep = (key == "cookie") ? "; " : ", ";
		std::string combined = values[0];
		for (size_t i = 1; i < values.size(); ++i)
			combined += sep + values[i];

		parsedReq.headers[key] = combined;
	}
	tmpHeaders.clear();
}

bool RequestParser::parseBody(CharDq& buf)
{
	if (parsedReq.isChunked) return parseChunkedBody(buf);
	return parsePlainBody(buf);
}
bool RequestParser::parsePlainBody(CharDq& buf)
{
	unsigned long long needed = static_cast<unsigned long long>(parsedReq.contentLength);
	if (buf.size() < needed) return false;

	parsedReq.body.assign(buf.begin(), buf.begin() + needed);
	buf.erase(buf.begin(), buf.begin() + needed);
	return true;
}
bool RequestParser::parseChunkedBody(CharDq& buf)
{
	while (true)
	{
		if (inTrailer)
		{
			size_t trailerEnd = HttpUtils::findCRLF(buf);
			if (trailerEnd == HttpUtils::npos) return false;
			buf.erase(buf.begin(), buf.begin() + trailerEnd + 2);
			if (trailerEnd == 0)
			{
				inTrailer = false;
				return true;
			}
			continue;
		}

		if (chunkRemaining == 0)
		{
			size_t crlf = HttpUtils::findCRLF(buf);
			if (crlf == HttpUtils::npos) return false;

			std::string sizeLine(buf.begin(), buf.begin() + crlf);
			buf.erase(buf.begin(), buf.begin() + crlf + 2);

			size_t ext = sizeLine.find(';');
			std::string hexStr = sizeLine.substr(0, ext == std::string::npos ? sizeLine.size() : ext);

			size_t s = hexStr.find_first_not_of(" \t");
			size_t e = hexStr.find_last_not_of(" \t");
			if (s == std::string::npos || hexStr.empty()) { statusCode = STATUS_BAD_REQUEST; return true; }
			hexStr = hexStr.substr(s, e - s + 1);

			for (size_t i = 0; i < hexStr.size(); ++i)
			{
				if (!HttpUtils::isHex(hexStr[i])) { statusCode = STATUS_BAD_REQUEST; return true; }
			}

			unsigned long long size = 0;
			for (size_t i = 0; i < hexStr.size(); ++i)
			{
				size = size * 16 + static_cast<unsigned long long>(HttpUtils::hexToInt(hexStr[i]));
				if (size > MAX_CLIENT_BODY_LENGTH) { statusCode = STATUS_PAYLOAD_TOO_LARGE; return true; }
			}

			if (size == 0)
			{
				inTrailer = true;
				continue;
			}
			chunkRemaining = static_cast<size_t>(size);
		}
		else
		{
			if (buf.size() < chunkRemaining + 2) return false;

			if (parsedReq.body.size() + chunkRemaining > MAX_CLIENT_BODY_LENGTH)
				{ statusCode = STATUS_PAYLOAD_TOO_LARGE; return true; }

			parsedReq.body.insert(parsedReq.body.end(), buf.begin(), buf.begin() + chunkRemaining);
			buf.erase(buf.begin(), buf.begin() + chunkRemaining + 2);
			chunkRemaining = 0;
		}
	}
}

void RequestParser::handleError()
{
	parsedReq.status = statusCode;
	tmpReqLine = ReqLine();
	tmpHeaders.clear();
}

void RequestParser::clear()
{
	parseState = REQ_STARTLINE;
	statusCode = STATUS_UNDEFINED;
	tmpReqLine = ReqLine();
	tmpHeaders.clear();
	parsedReq = Request();
	chunkRemaining = 0;
	inTrailer = false;
}

RequestParser::RequestParser() : parseState(REQ_STARTLINE), statusCode(STATUS_UNDEFINED), chunkRemaining(0), inTrailer(false) {}
RequestParser::~RequestParser() {}

void RequestParser::parse(CharDq& buf)
{
	if (parseState == REQ_STARTLINE)
	{
		if (!parseStartline(buf)) return ;
		if (statusCode != STATUS_UNDEFINED) { parseState = REQ_ERROR; handleError(); return; }
		parseState = REQ_HEADERS;
	}
	if (parseState == REQ_HEADERS)
	{
		if (!parseHeaders(buf)) return ;
		if (statusCode != STATUS_UNDEFINED) { parseState = REQ_ERROR; handleError(); return; }
		parseState = parsedReq.contentLength == -1 && !parsedReq.isChunked ? REQ_DONE : REQ_BODY;
	}
	if (parseState == REQ_BODY)
	{
		if (!parseBody(buf)) return ;
		if (statusCode != STATUS_UNDEFINED) { parseState = REQ_ERROR; handleError(); return; }
		parseState = REQ_DONE;
	}
}

ReqParseResult RequestParser::getState() const
{
	if (parseState == REQ_STARTLINE || parseState == REQ_HEADERS || parseState == REQ_BODY) return REQ_PARSE_INCOMPLETE;
	if (parseState == REQ_DONE) return REQ_PARSE_DONE;
	return REQ_PARSE_ERROR;
}
Request RequestParser::getRequest() const { return parsedReq; }
