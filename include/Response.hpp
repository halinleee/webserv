#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "type.hpp"
#include "HttpUtils.hpp"

#include <string>
#include <map>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <ctime>

struct CaseInsensitiveLess
{
	bool operator()(const std::string &a, const std::string &b) const
	{
		size_t n = std::min(a.size(), b.size());
		for (size_t i = 0; i < n; ++i)
		{
			unsigned char ca = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(a[i])));
			unsigned char cb = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(b[i])));
			if (ca != cb) return ca < cb;
		}
		return a.size() < b.size();
	}
};

struct Response
{
	typedef std::map<std::string, std::string, CaseInsensitiveLess> HeaderMap;

	Status statusCode;
	std::string statusText;

	HeaderMap headers;

	std::string body;

	Response() :
		statusCode(STATUS_OK), statusText(HttpUtils::getStatusText(STATUS_OK))
	{}

	explicit Response(Status code) :
		statusCode(code), statusText(HttpUtils::getStatusText(code))
	{}

	static std::string httpDate()
	{
		std::time_t now = std::time(NULL);
		std::tm tmResult;
		gmtime_r(&now, &tmResult);
		char buf[30];
		std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tmResult);
		return std::string(buf);
	}

	std::string toString(bool shouldClose, bool includeBody = true) const
	{
		HeaderMap outHeaders = headers;

		outHeaders.erase("Content-Length");
		if (statusCode != STATUS_NO_CONTENT)
		{
			std::ostringstream lenss;
			lenss << body.size();
			outHeaders["Content-Length"] = lenss.str();
		}
		if (shouldClose)
			outHeaders["Connection"] = "close";
		else
			outHeaders["Connection"] = "keep-alive";
		outHeaders["Date"] = httpDate();
		outHeaders["Server"] = "webserv/1.0";

		std::ostringstream oss;
		oss << "HTTP/1.1 " << static_cast<int>(statusCode) << " " << statusText << "\r\n";
		for (HeaderMap::const_iterator it = outHeaders.begin(); it != outHeaders.end(); ++it)
			oss << it->first << ": " << it->second << "\r\n";
		oss << "\r\n";
		if (includeBody)
			oss << body;
		return oss.str();
	}
};

#endif