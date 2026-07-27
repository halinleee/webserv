#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
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

	std::string toString(bool shouldClose) const
	{
		std::map<std::string, std::string> outHeaders = headers;

		if (statusCode == STATUS_NO_CONTENT)
			outHeaders.erase("Content-Length");
		else if (outHeaders.find("Content-Length") == outHeaders.end())
		{
			std::ostringstream lenss;
			lenss << body.size();
			outHeaders["Content-Length"] = lenss.str();
		}
		if (shouldClose)
			outHeaders["Connection"] = "close";

		std::ostringstream oss;
		oss << "HTTP/1.1 " << static_cast<int>(statusCode) << " " << statusText << "\r\n";
		for (std::map<std::string, std::string>::const_iterator it = outHeaders.begin(); it != outHeaders.end(); ++it)
			oss << it->first << ": " << it->second << "\r\n";
		oss << "\r\n" << body;
		return oss.str();
	}
};

#endif