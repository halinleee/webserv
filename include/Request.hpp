#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
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
	unsigned int port;
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

/*

- 일반 헤더는 하나만 유지 (마지막에 들어온 값으로) <- 잘 되고 있냐ㅏ?
- `Set-Cookie`나 `Cookie`, `X-Forwarded-For`와 같은 헤더는 여러 개 허용

*/