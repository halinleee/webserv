#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "type.hpp"
#include "RouteResult.hpp"
#include "Request.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

#include <string>

class Router
{
	private:
		static const LocationConfig* matchLocation(ServerConfig& config, const std::string& reqPath, std::string& matchedPrefix);
		static bool checkRedirect(const LocationConfig& loc, RouteResult& result);
		static bool checkMethod(const LocationConfig& loc, HttpMethod method, RouteResult& result);
		static std::string resolvePath(const LocationConfig& loc, const std::string& matchedPrefix, const std::string& reqPath);

	public:
		static RouteResult route(ServerConfig& config, const Request& req);
};

#endif