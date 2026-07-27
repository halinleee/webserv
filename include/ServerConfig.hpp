#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp"

#include <iosfwd>
#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <ctime>

class ServerConfig
{
	public:
		static const size_t BODY_SIZE_MAX = 5 * 1024 * 1024;
		static const size_t TIME_OUT_MAX = 180;
	
	private:
		timeValue timeConfig;
		size_t clientMaxBodySize;
		std::map<size_t, std::string> errorPages;
		std::map<std::string, LocationConfig> locations;
		std::string statusMessage;
		std::vector<std::string> prefixes;

	private:
		void setPrefixes(void);
		bool parseTimeOut(std::vector<std::string>& token);
		bool parseErrorPage(std::vector<std::string>& token);
		bool parseBody(const std::vector<std::string>& token);
		bool parseServerDirective(std::vector<std::string>& token, std::ifstream& configFile);
		parseStatus endSequenceValid(std::ifstream& configFile);

	public:
		parseStatus parseServerConfigBlock(std::ifstream &configFile);
		

	public:
		ServerConfig()
		{
			clientMaxBodySize = 1000000;
			statusMessage = "Default Error";
			timeConfig.connetionTimeOut = 60;
			timeConfig.readTimeout = 60;
			timeConfig.writeTimeout = 60;
			timeConfig.keepAliveTimeout = 75;
			timeConfig.cgiTimeout = 60;
		};
		const size_t& getClientMaxBodySize() const { return clientMaxBodySize; }
		const std::map<size_t, std::string>& getErrorPages() const { return errorPages; }
		const std::map<std::string, LocationConfig>& getLocations() const { return locations; }
		const std::string& getStatusMessage() const { return statusMessage; }
		const timeValue getTimeConfig() const { return timeConfig; }

	public:
		bool matching(const std::string& url);
		LocationConfig matchLocation;
		const std::string& getMatchedPrefix() const { return matchedPrefix; }

	private:
		std::string matchedPrefix;
};

#endif