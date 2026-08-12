#ifndef CGIPARSER_HPP
#define CGIPARSER_HPP

#include "HttpUtils.hpp"
#include "Response.hpp"
#include "ConfigParseUtils.hpp"

#include <iostream>

const size_t MAX_CGI_OUTPUT_LENGTH = 10 * 1024 * 1024;

class CgiParser
{
	public:
		Response parseCgiOutput(const std::string& cgiOutput);
};

#endif




