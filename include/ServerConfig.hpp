#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp"

#include <iosfwd>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

class ServerConfig
{
	private:
		time_t keepAliveTimeout;
		size_t clientMaxBodySize;
		std::map<size_t, std::string> errorPages;
		std::map<std::string, LocationConfig> locations;
		std::string statusMessage;
		std::vector<std::string> prefixes;

	private:
		void setPrefixes(void);
		bool parseKeepAlive(std::vector<std::string>& token);
		bool parseErrorPage(std::vector<std::string>& token);
		bool parseBody(const std::vector<std::string>& token);
		/**
		 * @brief server 블록 내부(indent 1) 지시어를 파싱한다.
		 * @details
		 * 허용 지시어:
		 * - client_max_body_size
		 * - error_page
		 * - location (location 블록 파싱은 LocationConfig에 위임)
		 * @param token 공백 기준으로 분리된 지시어 토큰
		 * @param configFile 열린 config 파일 스트림 (location 파싱 시 추가로 읽는다)
		 * @return 유효한 지시어이고 처리에 성공하면 true, 아니면 false
		 */
		bool parseServerDirective(std::vector<std::string>& token, std::ifstream& configFile);
		void endSequenceValid(std::ifstream& configFile);

	public:
		bool parseServerConfigBlock(std::ifstream &configFile);
		

	public:
		ServerConfig()
		{
			clientMaxBodySize = 1000000;
			statusMessage = "Default Error";
			keepAliveTimeout = 75;
		};
		const size_t& getClientMaxBodySize() const { return clientMaxBodySize; }
		int getKeepAliveTimeout() const { return keepAliveTimeout; }
		const std::map<size_t, std::string>& getErrorPages() const { return errorPages; }
		const std::map<std::string, LocationConfig>& getLocations() const { return locations; }
		const std::string& getStatusMessage() const { return statusMessage; }

	public:
		bool matching(const std::string& url);
		LocationConfig matchLocation;
};

#endif