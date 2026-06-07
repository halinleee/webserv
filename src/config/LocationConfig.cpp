#include "LocationConfig.hpp"
#include "Util.hpp"
#include <fstream>

bool LocationConfig::parseHttpMethod(const std::string &s, HttpMethod &out)
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

bool LocationConfig::parseLocationDir(std::vector<std::string> token)
{
	if (token[0] == "root")
	{
		if (token.size() != 2)
			return false;
		
		if (!isValidNormalizePath(token[1]))
			return false;

		root = token[1];
	}
	else if (token[0] == "index")
	{
		if (token.size() != 2)
			return false;
		if (!isValidFileName(token[1]))
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

		if (!isValidNormalizePath(token[1]))
			return false;
		uploadDir = token[1];
	}
	else if (token[0] == "return")
	{
		if (token.size() != 3)
			return false;

		size_t num = 0;
		if (!toInt(token[1], num))
			return false;

		if (!isValidNormalizePath(token[2]))
			return false;
		
		redirectCode = num;
		redirectPath = token[2];
	}

	else if (token[0] == "cgi_ext")
	{
		if (token.size() != 3)
			return false;

		if (!isValidNormalizePath(token[2]))
			return false;
		cgiExtension = token[1];
		cgiPath = token[2];
	}
	else
		return false;

	return true;
}

bool LocationConfig::parseLocationBlock(std::ifstream &configFile)
{
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
			return true;
		}
		if (indent == -1 || indent != 2)
			return false;
		
		removeIndent(line, '\t');
		std::vector<std::string> token = ftSplit(line, ' ');
		if (token.empty())
			return false;

		if (!parseLocationDir(token))
			return false;
	}
	return false;
}
