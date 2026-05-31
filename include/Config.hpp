#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerConfig.hpp"

#include <netinet/in.h>
#include <iosfwd>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

class Config
{
	private:
		std::map<in_port_t, ServerConfig> servers;
		std::string statusMessage;
		enum ParseStatus
		{
			PARSE_SERVER_END,
			PARSE_FILE_END,
			PARSE_ERROR
		};

	public:
		bool parseConfig();

	private:
		bool isValidListen(const std::vector<std::string>& token);
		ParseStatus parseServerBlock(std::ifstream& configFile);

	public:
		Config()
		{
			statusMessage = "Default Error";
		}
		const std::map<in_port_t, ServerConfig>& getConfig() const { return servers; }
		const std::string& getStatusMessage() const { return statusMessage; }
};

#endif