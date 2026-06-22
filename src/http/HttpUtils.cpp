#include "HttpUtils.hpp"

void HttpUtils::consumeLeadingCRLF(CharDq& buf)
{
	while (!buf.empty())
	{
		unsigned char c = buf.front();
		if (c == '\r' || c == '\n')
		{
			buf.pop_front();
			continue ;
		}
		break ;
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