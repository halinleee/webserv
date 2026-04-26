#include "../../include/Util.hpp"


bool isValidUriPath(std::string &path)
{
	if (path.empty())// nginx는 지시어의 경로가 '/'로 시작하지 않아도 url로 해석해서 '/'를 붙여서 처리함
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
	if (path.empty() || path[0] != '/')// nginx는 location의 prefix 경로가 '/'로 시작하지 않으면 에러로 처리함
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

/**
 * @struct removeIndent
 * @brief 들여쓰기 제거해주는 함수
 * 
 * 구분자가 아닌 문자를 만나면 그 위치부터 반환
 * 즉 문자열 앞부분 들여쓰기 지울때 사용한다
 */
bool removeIndent(std::string &value, char delim)
{
    size_t pos = 0;

    while (pos < value.size() && value[pos] == delim)
        ++pos;

    value.erase(0, pos);
    return true;
}

/**
 * @struct countIndent
 * @brief 탭 개수 세어주는 함수
 * 
 * '/t'는 탭 1개로 본다
 * 공백은 들여쓰기로 안본다
 * 
 * 반환값: 탭 개수
 */
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
			return false; //공백이 아닌 줄
	}
	return true;//공백인 줄
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
	return token;
}

//문자열을 숫자인지 판별해주는 함수
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

//string을 int형(정수)로 변환해주는 함수
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