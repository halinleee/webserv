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

	// STATIC, CGI
	std::string resolvedPath;

	// STATIC
	bool autoIndex;
	std::string index;

	// CGI
	std::string cgiInterpreter;

	// REDIRECT
	int redirectCode;
	std::string redirectPath;

	// ERROR
	int errorCode;
	std::set<HttpMethod> allowedMethods;

	RouteResult() :
		action(ACTION_ERROR), autoIndex(false), redirectCode(0), errorCode(0)
	{}
};


#endif