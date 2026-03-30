#include "../../include/Config.hpp"

bool removeChar (std::string &value, char delim)
{
    size_t pos = 0;

    while ((pos = value.find(delim, pos)) != std::string::npos)
    {
        value.erase(pos, 1);
    }
    return true;
}

/**
 * @struct countIndent
 * @brief 탭 개수 세어주는 함수
 * 
 * '/t'는 탭 1개로 본다
 * 
 * 공백 4개는 탭 1개로 보는 방식
 * 
 * 공백 4의 배수가 아닐 경우 에러처리(공백 8개 = 탭 2개)
 * 
 * 반환값: 탭 개수
 */
int countIndent(const std::string& line)
{
	std::string::const_iterator it  = line.begin();
	int indent = 0;
    int spaceCount = 0;
	while (it != line.end())
	{
		if (*it == '\t')
		{
			indent += spaceCount / 4;
			if (spaceCount % 4 != 0)
				return (-1);
			spaceCount = 0;
			++indent;
			++it;
		}
		else if (*it == ' ')
		{
			++spaceCount;
			++it;
		}
		else
			break;
	}
	indent += spaceCount / 4;
	if (spaceCount % 4 != 0)
		return (-1);
	return (indent);
	
}

/**
 * @struct isVBlankLine
 * @brief 문자열이 공백인지 판단하는 함수
 * 
 * 반환값 참(true): 공백인 줄
 * 
 * 반환값 거짓(false): 공백이 아닌 줄(문자열이 있는 줄)
 */
bool isBlankLine(const std::string &line)
{
	for(std::string::const_iterator it = line.begin(); it != line.end(); ++it)
	{
		if (!std::isspace(static_cast<unsigned char>(*it)))
			return (false); //공백이 아닌 줄
	}
	return (true);//공백인 줄
}

//구분자 단위로 잘라담는 split
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
	return (token);
}

//숫자인지 판별해주는 함수
bool isNumber(const std::string &s)
{
	if (s.empty())
		return (false);
	std::string::const_iterator it = s.begin();
	for (; it != s.end(); ++it)
	{
		if (!std::isdigit(static_cast<unsigned char>(*it)))
			return (false);
	}
	return (true);
}

//string을 int형(정수)로 변환해주는 함수
bool toInt (const std::string &s, unsigned int &value)
{
	if (!isNumber(s))
		return (false);

	std::stringstream ss(s);
	ss >> value;
	if (!ss || !ss.eof())
		return (false);
	return (true);
}