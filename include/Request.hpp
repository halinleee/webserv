#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <netinet/in.h>
#include "type.hpp"

struct Request
{
	// state
	Status status;

	// request line
	HttpMethod method;
	std::string path;
	std::string query;

	// headers
	std::string host;
	in_port_t port;
	std::map<std::string, std::string> headers;
	
	// body
	long long contentLength;
	bool isChunked;
	std::string body;

	Request() :
		status(STATUS_UNDEFINED), method(METHOD_INVALID), port(80),
		contentLength(-1), isChunked(false)
	{}
	bool isValid() const { return status < 400; }
};

#endif