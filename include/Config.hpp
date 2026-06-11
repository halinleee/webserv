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

	public:
		bool parseConfig();

	private:
		bool isValidListen(const std::vector<std::string>& token, size_t& num);
		parseStatus parseServerBlock(std::ifstream& configFile);

	public:
		Config()
		{
			statusMessage = "Default Error";
		}
		const std::map<in_port_t, ServerConfig>& getConfig() const { return servers; }
		const std::string& getStatusMessage() const { return statusMessage; }
};

#endif