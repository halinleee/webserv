#include "../../include/Config.hpp"

bool parseHttpMethod(const  std::string &s, HttpMethod &out)
{
	if (s == "GET") 
	{
		out = METHOD_GET;
		return true;
	}
	if (s == "POST")
	{
		out = METHOD_POST;
		return true;
	}
	if (s == "DELETE")
	{
		out = METHOD_DELETE;
		return true;
	}
	return false;
}

bool Config::locationValidate(unsigned int key, const std::vector<std::string>& locationToken, std::ifstream& configFile)
{
	if (locationToken.size() != 2)
		return false;

	const std::string &prefix = locationToken[1];
	servers[key].locations[prefix];

	std::string line;
	while (true)
	{
		std::streampos pos = configFile.tellg();
		if (!std::getline(configFile, line))
			break;
		
		if (isBlankLine(line))
			continue;
		
		int indent = countIndent(line);
		if (indent == -1)
			return false;
		
		if (indent == 0 || indent == 1)
		{
			configFile.clear();
			configFile.seekg(pos);
			return true;
		}
		if (indent != 2)
			return false;
		
		removeChar(line, '\t');
		std::vector<std::string> token = ftSplit(line, ' ');
		if (token.empty())
			return false;
	
		if (token[0] == "root")
		{
			if (token.size() != 2)
				return false;
			servers[key].locations[prefix].setRoot(token[1]);
		}
		else if (token[0] == "index")
		{
			if (token.size() != 2)
				return false;
			servers[key].locations[prefix].setIndex(token[1]);
		}
		
		else if (token[0] == "methods")
		{
			if (token.size() < 2)
				return false;
			servers[key].locations[prefix].clearMethods();
			for (size_t i = 1; i < token.size(); ++i)
			{
				HttpMethod method;
				if (!parseHttpMethod(token[i], method))
					return false;
				servers[key].locations[prefix].setMethod(method);
			}
		}
		else if (token[0] == "autoindex")
		{
			if (token[1] == "on")
				servers[key].locations[prefix].setAutoIndex(true);
			else if (token[1] == "off")
				servers[key].locations[prefix].setAutoIndex(false);
			else
				return false;
		}
		else if (token[0] == "upload_dir")
		{
			if (token.size() != 2)
				return false;
			servers[key].locations[prefix].setUploadDir(token[1]);
		}
		else if (token[0] == "return")
		{
			if (token.size() != 3)
				return false;
			unsigned int num = 0;
			if (!toInt(token[1], num))
				return false;
			servers[key].locations[prefix].setRedirect(num, token[2]);
		}
		else if (token[0] == "cgi_ext")
		{
			if (token.size() != 3)
				return false;
			servers[key].locations[prefix].setCgi(token[1], token[2]);
		}
		else
			return false;
	}
	return false;
}

bool Config::errorPageValidate(unsigned int key, const std::vector<std::string>& token)
{
	if (token.size() != 3)
		return false;

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return false;
	
	servers[key].setErrorPages(num, token[2]);
	return true;
}


bool Config::listenBodyValidate(unsigned int key, const std::vector<std::string>& token, size_t max)
{
	if (token.size() != 2)
		return false;

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return false;

	if (num == 0 || num > max)
		return false;

	if (token[0] == "client_max_body_size")
		servers[key].setClientMaxBodySize(static_cast<size_t>(num));
	return true;
}

/**
 * @struct serverDirectiveValidate
 * @brief 서버 설정 지시어들의 유효 검사를 진입하는 함수
 */
bool Config::serverDirectiveValidate(unsigned int key, const std::vector<std::string>& token, std::ifstream& configFile)
{	
	if (token[0] == "client_max_body_size")
		return listenBodyValidate(key, token, 10000000);
	else if (token[0] == "error_page")
		return (errorPageValidate(key, token));
	else if (token[0] == "location")
		return (locationValidate(key, token, configFile));
	
	std::cout << "Config error: Invalid server block format(serverDirectiveValidate)" << std::endl;
	return false;
}

//end 연속성 검사 함수
int Config::isEndSequenceValid(std::ifstream &configFile)
{
	std::string nextLine;
	while (true)
	{
		std::streampos nextPos = configFile.tellg();
		if (!std::getline(configFile, nextLine))
			return 1;
		if (isBlankLine(nextLine))
			continue;
		std::string tmp = nextLine;
		removeChar(tmp, '\t');
		std::vector<std::string> nextToken = ftSplit(tmp, ' ');
		if (!nextToken.empty() && nextToken[0] == "server")
		{
			configFile.clear();
			configFile.seekg(nextPos);
			return 0;
		}
		std::cout << "Config error: Invalid server block format" << std::endl;
		return -1;
	}
}

/**
 * @struct validateConfig
 * @brief configfile 문법 유효성 검사 함수
 * 
 * 오타, 블록 단위, 지시어 등 문법이 유효한지 검사하여
 * 유효한 configfile이면 true
 * 오류있는 configfile이면 false를 반환한다
 */
int Config::validateConfig(std::ifstream &configFile)
{
	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;
		break;
	}
	
	if (configFile.eof() && servers.empty())
	{
		std::cout << "Config error: config file is empty" << std::endl;
		return -1;
	}
	else if (configFile.eof())
		return 1;
	
	std::vector<std::string> token = ftSplit(configLine, ' ');
	if (token.empty())
	{
		std::cout << "Config error: token is empty" << std::endl;
		return -1;
	}

	if (token[0] != "server" || !listenBodyValidate(0, token, 9999))
	{
		std::cout << "Config error: server or port error" << std::endl;
		return -1;
	}

	unsigned int key = 0;
	if (!toInt(token[1], key))
	{
		std::cout << "Config error: port config error" << std::endl;
		return (-1);
	}
		
	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;

		int indent = countIndent(configLine);
		
		
		if (indent != 1)
		{
			std::cout << "Config error: indent error" << std::endl;
			return -1;
		}

		if (indent == 1)
		{
			removeChar(configLine, '\t');
			token = ftSplit(configLine, ' ');
			if (token.empty())
			{
				std::cout << "Config error: token is empty" << std::endl;
				return -1;
			}
			if (!serverDirectiveValidate(key, token, configFile))
			{
				std::cout << "Config error: Invalid server block format(serverDirectiveValidate)" << std::endl;
				return -1;
			}
		}

		if (indent == 0)
		{
			if (configLine == "end")
				return (isEndSequenceValid(configFile));
			else
			{
				std::cout << "Config error: Invalid server block format" << std::endl;
				return -1;
			}
		}
	}
	std::cout << "Config error: Invalid server block format(validateConfig)" << std::endl;
	return -1;
	// return (1); //모든 서버 검사를 마쳤을때
	// return (0); //한개의 서버 검사를 마쳤을때
	// return (-1); //에러
}

//location 부분 / 유효성 검사
//location이 한개 이상있어야 한다
//validateConfig에서도 location이 한번 이상 있는지 확인
//지시어에 tab 넣었을때 왜 되냐(split 탭 들어갔을때 무시하는거 검사)