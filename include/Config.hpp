# ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <set>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>

#include "ServerConfig.hpp"

/** 
 * @class Config
 * @brief 여러 서버 설정을 관리하는 최상위 구조체
 */
class Config
{
	private:
		/**
		 * @var servers
		 * @brief 여러 서버 설정을 관리하는 최상위 맵
		 * 
		 * - key : 포트 번호(int)
		 * 
		 * - value : 해당 포트에서 동작하는 서버 설정(ServerConfig)
		 * 
		 * 각 ServerConfig는 다음과 같은 정보를 포함한다
		 * 
		 * - client_max_body_size
		 * 
		 * - error_page
		 * 
		 * - location 설정 등
		 * 
		 * - 기타 서버 관련 설정
		 */
		std::map<unsigned int, ServerConfig> servers;
		std::string statusMessage;

	private:
		bool parseListen(const std::vector<std::string>& token, size_t max);	
		int  parseServerBlock(std::ifstream& configFile);
		
	public:
		Config();
		std::map<unsigned int, ServerConfig> getConfig() { return servers; }
		std::string getStatusMessage() const { return statusMessage; }
	
};



# endif