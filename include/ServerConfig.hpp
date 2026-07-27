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
		std::time_t keepAliveTimeout;
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
		bool parseServerDirective(std::vector<std::string>& token, std::ifstream& configFile);
		parseStatus endSequenceValid(std::ifstream& configFile);

	public:
		parseStatus parseServerConfigBlock(std::ifstream &configFile);
		

	public:
		ServerConfig()
		{
			clientMaxBodySize = 1000000;
			statusMessage = "Default Error";
			keepAliveTimeout = 75;
		};
		const size_t& getClientMaxBodySize() const { return clientMaxBodySize; }
		std::time_t getKeepAliveTimeout() const { return keepAliveTimeout; }
		const std::map<size_t, std::string>& getErrorPages() const { return errorPages; }
		const std::map<std::string, LocationConfig>& getLocations() const { return locations; }
		const std::string& getStatusMessage() const { return statusMessage; }

	public:
		bool matching(const std::string& url);
		LocationConfig matchLocation;
		std::string matchPrefix;
};

#endif