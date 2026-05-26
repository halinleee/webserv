#include "../../include/Util.hpp"
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>
#include <cctype>

/**
 * @brief base 경로를 기준으로 path를 합쳐 경로를 만든다.
 *
 * @details
 * - path가 '/'로 시작하면 이미 절대경로로 간주하고 그대로 반환한다.
 * - 상대경로이면 base 뒤에 path를 이어붙여 반환한다.
 *
 * @param base 기준 경로
 * @param path 이어붙일 경로
 * @return 생성된 경로 문자열
 */
/*
static std::string makeAbsolutePath(const std::string &base, const std::string &path)
{
	if (path[0] == '/')
		return path;

	if (base.empty())
		return path;
	if (base[base.size() - 1] == '/')
		return base + path;
	return base + "/" + path;
}
	*/


bool isValidRoot(const std::string &path)
{
	struct stat st;

	if (stat(path.c_str(), &st) != 0)
		return false;

	if (!S_ISDIR(st.st_mode))
		return false;

	if (access(path.c_str(), R_OK) != 0)
		return false;

	return true;
}


bool isValidUriPath(std::string &path)
{
	if (path.empty())// 지시어의 경로가 '/'로 시작하지 않아도 url로 해석함
		return false;
	
	std::string str;
	bool prevSlash = false;
	for (size_t i = 0; i < path.size(); ++i)
	{
		const unsigned char ch = static_cast<unsigned char>(path[i]);

		if (std::isspace(ch)) 
			return false;

		if (path[i] == '/')
		{
			if (prevSlash)
				continue;
			prevSlash = true;
		}
		else
			prevSlash = false;
		str.push_back(path[i]);
	}
	path.swap(str);
	return true;
}

bool isValidPrefix(std::string &path)
{
	if (path.empty() || path[0] != '/')// location의 prefix 경로가 '/'로 시작하지 않으면 에러로 처리함
		return false;
	
	std::string str;
	bool prevSlash = false;
	for (size_t i = 0; i < path.size(); ++i)
	{
		const unsigned char ch = static_cast<unsigned char>(path[i]);

		if (std::isspace(ch)) 
			return false;

		if (path[i] == '/')
		{
			if (prevSlash)
				continue;
			prevSlash = true;
		}
		else
			prevSlash = false;
		str.push_back(path[i]);
	}
	path.swap(str);
	return true;
}


void removeIndent(std::string &value, char delim)
{
    size_t pos = 0;

    while (pos < value.size() && value[pos] == delim)
        ++pos;

    value.erase(0, pos);
    return ;
}


int countIndent(const std::string& line)
{
	std::string::const_iterator it  = line.begin();
	int indent = 0;
	while (it != line.end())
	{
		if (*it == '\t')
		{
			++indent;
			++it;
		}
		else if (*it == ' ')
			return -1;
		else
			break;
	}
	return indent;
}


bool isBlankLine(const std::string &line)
{
	for(std::string::const_iterator it = line.begin(); it != line.end(); ++it)
	{
		if (!std::isspace(static_cast<unsigned char>(*it)))
			return false; //공백이 아닌 줄
	}
	return true;//공백인 줄
}


std::vector<std::string> ftSplit(const std::string& line, char delim)
{
	std::vector<std::string> token;
	std::size_t start = 0;
	while (true)
	{
		std::size_t pos = line.find(delim, start);
		if (pos == std::string::npos)
		{
			if (start < line.size())
				token.push_back(line.substr(start));
			break;
		}
		if (pos > start)
			token.push_back(line.substr(start, pos - start));
		start = pos + 1;
	}
	return token;
}


bool isNumber(const std::string &s)
{
	if (s.empty())
		return false;
	std::string::const_iterator it = s.begin();
	for (; it != s.end(); ++it)
	{
		if (!std::isdigit(static_cast<unsigned char>(*it)))
			return false;
	}
	return true;
}


bool toInt (const std::string &s, unsigned int &value)
{
	if (!isNumber(s))
		return false;

	std::stringstream ss(s);
	ss >> value;
	if (!ss || !ss.eof())
		return false;
	return true;
}