#ifndef ROUTERESULT_HPP
#define ROUTERESULT_HPP

#include "type.hpp"

#include <string>
#include <set>

enum RouteAction
{
	ACTION_STATIC,
	ACTION_CGI,
	ACTION_REDIRECT,
	ACTION_ERROR
};

struct RouteResult
{
	RouteAction action;

	std::string resolvedPath;

	bool autoIndex;
	std::string index;

	std::string cgiInterpreter;

	int redirectCode;
	std::string redirectPath;

	int errorCode;
	std::set<HttpMethod> allowedMethods;

	RouteResult() :
		action(ACTION_ERROR), autoIndex(false), redirectCode(0), errorCode(0)
	{}
};


#endif