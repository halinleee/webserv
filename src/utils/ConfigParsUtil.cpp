#include "Util.hpp"
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>
#include <cctype>

bool isValidFileName(const std::string &file)
{
	if (file.empty())
    	return false;
	for(size_t i = 0; i < file.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(file[i]);

		if (!std::isalnum(ch) && ch != '_' && ch != '.' )
			return false;
	}
	return true;
}


bool isValidNormalizePath(std::string &path)
{
	if (path.empty() || path[0] != '/')//경로가 '/'로 시작하지 않으면 에러로 처리함
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


bool toInt (const std::string &s, size_t &value)
{
	if (!isNumber(s))
		return false;

	std::stringstream ss(s);
	ss >> value;
	if (!ss || !ss.eof())
		return false;
	return true;
}