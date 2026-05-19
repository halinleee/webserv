# ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <set>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>

#include "LocationConfig.hpp"


/**
 * @class ServerConfig
 * @brief 설정 파일 내 하나의 server 블록을 표현한 구조체
 */
class ServerConfig
{
	private:
		size_t clientMaxBodySize;
		std::map<unsigned int, std::string> errorPages;
		std::map<std::string, LocationConfig> locations;
		std::string statusMessage;
		std::vector<std::string> prefixes;
		std::string prefixPath;

	private:
		bool parseErrorPage(std::vector<std::string> &token);
		bool parseBody(const std::vector<std::string> &token, size_t max);
		bool parseServerDirective(std::vector<std::string> &token, std::ifstream& configFile); 
		void endSequenceValid(std::ifstream &configFile); 
		void setPrefixes(void);

	public:
		ServerConfig()
		{
			clientMaxBodySize = 1000000;
		};
		ServerConfig(std::ifstream &configFile, std::string configLine);
		const size_t getClientMaxBodySize() const { return clientMaxBodySize; }
		const std::map<unsigned int, std::string> getErrorPages() const { return errorPages; }
		const std::map<std::string, LocationConfig> getLocations() const { return locations; }
		const std::string getStatusMessage() const { return statusMessage; }
		const std::string getPrefixPath() const { return prefixPath; }

	public:
		bool Matching(const std::string &url);
		LocationConfig matchLocation;
	
};

# endif