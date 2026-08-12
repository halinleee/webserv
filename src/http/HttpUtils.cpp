#include "HttpUtils.hpp"

#include <cctype>

void HttpUtils::consumeLeadingCRLF(CharDq& buf, size_t maxBlankLines)
{
	for (size_t skipped = 0; skipped < maxBlankLines; ++skipped)
	{
		if (buf.size() < 2 || buf[0] != '\r' || buf[1] != '\n')
			break ;
		buf.erase(buf.begin(), buf.begin() + 2);
	}
}

size_t HttpUtils::findCRLF(const CharDq& buf)
{
	if (buf.size() < 2)
		return (HttpUtils::npos);

	for(size_t i = 0; i + 1 < buf.size(); ++i)
	{
		if (buf[i] == '\r' && buf[i + 1] == '\n')
			return (i);
	}
	return HttpUtils::npos;
}

size_t HttpUtils::findCRLFCRLF(const CharDq& buf)
{
	if (buf.size() < 4)
		return (HttpUtils::npos);

	for(size_t i = 0; i + 3 < buf.size(); ++i)
	{
		if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
			return (i);
	}
	return HttpUtils::npos;
}

size_t HttpUtils::findBareLF(const CharDq& buf)
{
	for (size_t i = 0; i < buf.size(); ++i)
	{
		if (buf[i] != '\n')
			continue;
		if (i == 0 || buf[i - 1] != '\r')
			return i;
		return HttpUtils::npos;
	}
	return HttpUtils::npos;
}

std::string HttpUtils::extractLine(CharDq& buf, size_t end_pos, size_t end_size)
{
	std::string line(buf.begin(), buf.begin() + end_pos);
	buf.erase(buf.begin(), buf.begin() + end_pos + end_size);
	return line;
}

bool HttpUtils::hasCR(const std::string& line)
{
	for(size_t i = 0; i < line.size(); ++i)
	{
		if (line[i] == '\r')
			return true;
	}
	return false;
}

bool HttpUtils::isHex(const char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

bool HttpUtils::hasConsecutiveSlashes(const std::string& str)
{
	if (str.empty()) return false;

	for(size_t i = 1; i < str.size(); ++i)
	{
		if (str[i] == '/' && str[i - 1] == '/') return true;
	}
	return false;
}

bool HttpUtils::hasDotSegments(const std::string& str)
{
	size_t start = 0;
	while (start < str.size())
	{
		size_t end = str.find('/', start);
		if (end == std::string::npos) end = str.size();

		std::string segment = str.substr(start, end - start);
		if (segment == "." || segment == "..") return true;
		start = end + 1;
	}
	return false;
}

int HttpUtils::hexToInt(const char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return -1;
}

bool HttpUtils::isTchar(unsigned char c)
{
    if (std::isalnum(c)) return true;
    switch (c)
    {
        case '!': case '#': case '$': case '%': case '&':
        case '\'': case '*': case '+': case '-': case '.':
        case '^': case '_': case '`': case '|': case '~':
            return true;
        default:
            return false;
    }
}

bool HttpUtils::isVcharSpTab(unsigned char c)
{
	if (c == ' ' || c == '\t') return true;
	return c >= 0x21 && c <= 0x7E;
}

std::string HttpUtils::getStatusText(Status code)
{
	switch (code)
	{
		case STATUS_OK: return "OK";
		case STATUS_CREATED: return "Created";
		case STATUS_NO_CONTENT: return "No Content";

		case STATUS_MOVED_PERMANENTLY: return "Moved Permanently";
		case STATUS_FOUND: return "Found";
		case STATUS_SEE_OTHER: return "See Other";

		case STATUS_BAD_REQUEST: return "Bad Request";
		case STATUS_FORBIDDEN: return "Forbidden";
		case STATUS_NOT_FOUND: return "Not Found";
		case STATUS_METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case STATUS_REQUEST_TIMEOUT: return "Request Timeout";
		case STATUS_PAYLOAD_TOO_LARGE: return "Payload Too Large";
		case STATUS_URI_LONG: return "URI Too Long";
		case STATUS_HEADER_TOO_LARGE: return "Request Header Fields Too Large";

		case STATUS_INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case STATUS_NOT_IMPLEMENTED: return "Not Implemented";
		case STATUS_BAD_GATEWAY: return "Bad Gateway";
		case STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
		case STATUS_GATEWAY_TIMEOUT: return "Gateway Timeout";
		case STATUS_HTTP_VERSION: return "HTTP Version Not Supported";

		case STATUS_UNDEFINED: default: return "Unknown";
	}
}

std::string HttpUtils::getMethodName(HttpMethod method)
{
	switch (method)
	{
		case METHOD_GET: return "GET";
		case METHOD_POST: return "POST";
		case METHOD_DELETE: return "DELETE";
		case METHOD_PUT: return "PUT";
		case METHOD_PATCH: return "PATCH";
		case METHOD_HEAD: return "HEAD";
		case METHOD_OPTIONS: return "OPTIONS";
		case METHOD_TRACE: return "TRACE";
		case METHOD_CONNECT: return "CONNECT";
		case METHOD_INVALID: default: return "";
	}
}

std::string HttpUtils::getMimeType(const std::string& path)
{
	size_t slash = path.find_last_of('/');
	size_t dot = path.find_last_of('.');

	if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
		return "application/octet-stream";

	std::string ext = path.substr(dot + 1);
	for (size_t i = 0; i < ext.size(); ++i)
		ext[i] = std::tolower(static_cast<unsigned char>(ext[i]));

	if (ext == "html" || ext == "htm") return "text/html";
	if (ext == "css") return "text/css";
	if (ext == "js") return "application/javascript";
	if (ext == "json") return "application/json";
	if (ext == "txt") return "text/plain";
	if (ext == "png") return "image/png";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "gif") return "image/gif";
	if (ext == "svg") return "image/svg+xml";
	if (ext == "ico") return "image/x-icon";
	if (ext == "pdf") return "application/pdf";

	return "application/octet-stream";
}

std::string HttpUtils::toLower(const std::string& s)
{
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = std::tolower(static_cast<unsigned char>(result[i]));
	return result;
}

std::string HttpUtils::htmlEscape(const std::string& s)
{
	std::string result;
	for (size_t i = 0; i < s.size(); ++i)
	{
		switch (s[i])
		{
			case '&': result += "&amp;"; break;
			case '<': result += "&lt;"; break;
			case '>': result += "&gt;"; break;
			case '"': result += "&quot;"; break;
			case '\'': result += "&#39;"; break;
			default: result += s[i];
		}
	}
	return result;
}

std::string HttpUtils::urlEncode(const std::string& s)
{
	static const char* hexDigits = "0123456789ABCDEF";
	std::string result;

	for (size_t i = 0; i < s.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(s[i]);
		bool unreserved = std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~';

		if (unreserved)
			result += static_cast<char>(c);
		else
		{
			result += '%';
			result += hexDigits[(c >> 4) & 0x0F];
			result += hexDigits[c & 0x0F];
		}
	}
	return result;
}

std::string HttpUtils::joinPath(const std::string& base, const std::string& tail)
{
	bool baseEndsSlash = !base.empty() && base[base.size() - 1] == '/';
	bool tailStartsSlash = !tail.empty() && tail[0] == '/';

	std::string joined = base;

	if (baseEndsSlash && tailStartsSlash)
		joined += tail.substr(1);
	else if (!baseEndsSlash && !tailStartsSlash && !tail.empty())
	{
		joined += "/";
		joined += tail;
	}
	else
		joined += tail;

	return joined;
}