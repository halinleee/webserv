#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include "type.hpp"
#include "HttpUtils.hpp"

struct Response
{
	// status line
	Status statusCode;
	std::string statusText;

	// headers
	std::map<std::string, std::string> headers;

	// body
	std::string body;

	Response() :
		statusCode(STATUS_OK), statusText(HttpUtils::getStatusText(STATUS_OK))
	{}

	explicit Response(Status code) :
		statusCode(code), statusText(HttpUtils::getStatusText(code))
	{}
};

#endif