#include "Router.hpp"
#include "HttpUtils.hpp"
#include "type.hpp"
#include <set>
#include <string>

const LocationConfig* Router::matchLocation(ServerConfig& config, const std::string& reqPath, std::string& matchedPrefix)
{
	if (!config.matching(reqPath))
		return NULL;

	matchedPrefix = config.getMatchedPrefix();
	return &config.matchLocation;
}

bool Router::checkRedirect(const LocationConfig& loc, RouteResult& result)
{
	if (loc.getRedirectCode() == static_cast<size_t>(STATUS_UNDEFINED))
		return false;

	result.action = ACTION_REDIRECT;
	result.redirectCode = static_cast<int>(loc.getRedirectCode());
	result.redirectPath = loc.getRedirectPath();
	return true;
}

bool Router::checkMethod(const LocationConfig& loc, HttpMethod method, RouteResult& result)
{
	const std::set<HttpMethod>& methods = loc.getMethods();

	if (methods.find(method) != methods.end())
		return true;

	result.action = ACTION_ERROR;
	result.errorCode = STATUS_METHOD_NOT_ALLOWED;
	result.allowedMethods = methods;
	return false;
}

std::string Router::resolvePath(const LocationConfig& loc, const std::string& matchedPrefix, const std::string& reqPath)
{
	const std::string& alias = loc.getAlias();

	// alias
	if (!alias.empty())
	{
		std::string tmp;
		if (reqPath.size() >= matchedPrefix.size())
			tmp = reqPath.substr(matchedPrefix.size());
		return HttpUtils::joinPath(alias, tmp);
	}

	// root
	return HttpUtils::joinPath(loc.getRoot(), reqPath);
}

RouteResult Router::route(ServerConfig& config, const Request& req)
{
	RouteResult result;
	std::string matchedPrefix;

	const LocationConfig* loc = matchLocation(config, req.path, matchedPrefix);
	if (loc == NULL)
	{
		result.action = ACTION_ERROR;
		result.errorCode = STATUS_NOT_FOUND;
		return result;
	}

	if (checkRedirect(*loc, result))
		return result;

	if (!checkMethod(*loc, req.method, result))
		return result;

	std::string resolved = resolvePath(*loc, matchedPrefix, req.path);
	const std::string& cgiExt = loc->getCgiExtension();

	if (!cgiExt.empty() && resolved.size() >= cgiExt.size() &&
		resolved.compare(resolved.size() - cgiExt.size(), cgiExt.size(), cgiExt) == 0)
	{
		result.action = ACTION_CGI;
		result.resolvedPath = resolved;
		result.cgiInterpreter = loc->getCgiPath();
	}
	else
	{
		result.action = ACTION_STATIC;
		result.resolvedPath = resolved;
		result.autoIndex = loc->getAutoIndex();
		result.index = loc->getIndex();
	}

	return result;
}