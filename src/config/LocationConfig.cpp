#include "../../include/LocationConfig.hpp"

static bool parseHttpMethod(const std::string &s, HttpMethod &out)
{
	if (s == "GET") 
	{
		out = METHOD_GET;
		return true;
	}
	else if (s == "POST")
	{
		out = METHOD_POST;
		return true;
	}
	else if (s == "DELETE")
	{
		out = METHOD_DELETE;
		return true;
	}
	return false;
}

/**
 * @struct parseLocDir
 * @brief location 지시어 부분 유효성 검사 및 값 세팅
 * 
 */
bool LocationConfig::parseLocDir(std::vector<std::string> token)
{
	if (token[0] == "root")
	{
		if (token.size() != 2)
			return false;
		
		if (!isValidUriPath(token[1]))
			return false;
		root = token[1];
	}
	else if (token[0] == "index")
	{
		if (token.size() != 2)
			return false;
		
		if (!isValidUriPath(token[1]))
			return false;
		index = token[1];
	}
	else if (token[0] == "methods")
	{

		if (token.size() < 2)
			return false;
		
		methods.clear();
		for (size_t i = 1; i < token.size(); ++i)
		{
			HttpMethod method;
			if (!parseHttpMethod(token[i], method))
				return false;
			methods.insert(method);
		}
	}
	else if (token[0] == "autoindex")
	{
		if (token.size() != 2)
			return false;

		if (token[1] == "on")
			autoIndex = true;
		else if (token[1] == "off")
			autoIndex = false;
		else
			return false;
	}
	else if (token[0] == "upload_dir")
	{
		if (token.size() != 2)
			return false;

		if (!isValidUriPath(token[1]))
			return false;
		uploadDir = token[1];
	}
	else if (token[0] == "return")
	{
		if (token.size() != 3)
			return false;
		
		unsigned int num = 0;
		if (!toInt(token[1], num))
			return false;

		if (!isValidUriPath(token[2]))
			return false;
		redirectStatusCode = num; 
		redirectPath = token[2];
	}
	else if (token[0] == "cgi_ext")
	{
		if (token.size() != 3)
			return false;

		if (!isValidUriPath(token[2]))
			return false;
		cgiExtension = token[1];
		cgiPath = token[2];
	}
	else
		return false;

	return true;
}

/**
 * @struct LocationConfig
 * @brief locationconfig의 일반생성자이자 location 블록 안에 지시어들의 들여쓰기를 검사하고 값을 담는 함수를 부른다.
 * 
 */
LocationConfig::LocationConfig(std::ifstream &configFile)
{
	redirectStatusCode = 0;
	std::string line;
	while (true)
	{
		std::streampos pos = configFile.tellg();
		if (!std::getline(configFile, line))
			break;
		
		if (isBlankLine(line))
			continue;
		
		int indent = countIndent(line);
		
		if (indent == 0 || indent == 1)
		{
			configFile.clear();
			configFile.seekg(pos);
			status = true;
			return ;
		}
		if (indent == -1 || indent != 2) //indent -1은 countIndent의 에러처리
		{
			status = false;
			return ;
		}
		
		removeIndent(line, '\t');
		std::vector<std::string> token = ftSplit(line, ' ');
		if (token.empty())
		{
			status = false;
			return ;
		}
		if (!parseLocDir(token))
		{
			status = false;
			return ;
		}
	}
	status = false;
	return ;
}