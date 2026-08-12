#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "type.hpp"

#include <string>
#include <map>
#include <netinet/in.h>

struct Request
{
	Status status;

	HttpMethod method;
	std::string path;
	std::string query;

	std::string host;
	in_port_t port;
	std::map<std::string, std::string> headers;

	long long contentLength;
	bool isChunked;
	std::string body;

	Request() :
		status(STATUS_UNDEFINED), method(METHOD_INVALID), port(80),
		contentLength(-1), isChunked(false)
	{}
};

#endif