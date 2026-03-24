//#include "Config.hpp"
#include <fstream> //ifstream
#include <iostream>
#include <string>

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

/**
 * @struct validateConfig
 * @brief configfile 문법 유효성 검사 함수
 * 
 * 오타, 블록 단위, 지시어 등 문법이 유효한지 검사하여
 * 유효한 configfile이면 true
 * 오류있는 configfile이면 false를 반환한다
 */
int validateConfig(std::ifstream& configFile)
{
	//1. sever가 들어오면 그 다음 토큰에서는 무조건 탭이 1개 이상 들어가야 함
	//1개 이상 탭이 없으면 에러 or 다음서버

	//3.25할일: 빈줄 검사 할때 그 다음줄로 밀리는 문제 해결
	// 구조 전체적으로 다시 보기

	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (!isBlankLine(configLine))
			break;
	}

	std::getline(configFile, configLine);
	int indentCount = countIndent(configLine);
	if (configLine == "server" && indentCount == 0)
	{
		while (std::getline(configFile, configLine) && configLine != "end")
		{
			if (!isBlankLine(configLine))
			{
				
			}
		}
		if (configFile.eof())
			return (1);
		return (0);
		
	}
	return (-1); //에러
}


// configFile.clear();
// configFile.seekg(0, std::ios::beg);


int main()
{
	std::ifstream configFile("config/webserv.conf");
	if (!configFile.is_open())
		return (-1);

	//유효성 검사
	while (true)
	{
		int res = validateConfig(configFile);
		if (res == -1)//에러
			return (-1);
		if (res == 1)//모든 서버 검사를 마쳤을 때
			break;
	}

	
	return (0);
}
