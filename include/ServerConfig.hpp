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

	private:
		bool parseErrorPage(const std::vector<std::string> &token);
		bool parseBody(const std::vector<std::string> &token, size_t max);
		bool parseServerDirective(const std::vector<std::string> &token, std::ifstream& configFile); 
		void  EndSequenceValid(std::ifstream &configFile);

	public:
		ServerConfig() {};
		ServerConfig(std::ifstream &configFile, std::string configLine);
		size_t getClientMaxBodySize() const { return clientMaxBodySize; }
		std::map<unsigned int, std::string> getErrorPages() const { return errorPages; }
		std::map<std::string, LocationConfig> getLocations() const { return locations; }
		std::string getStatusMessage() const { return statusMessage; }
	
};


# endif