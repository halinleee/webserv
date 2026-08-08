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

/**
 * @brief HTTP 헤더 필드명은 대소문자를 구분하지 않으므로(RFC 7230 §3.2),
 *        Response::headers 맵의 키 비교에 사용하는 대소문자 무관 비교 함수객체.
 */
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
	/// 대소문자 무관 헤더 맵 타입 (예: "Content-Length"와 "content-length"는 동일 취급)
	typedef std::map<std::string, std::string, CaseInsensitiveLess> HeaderMap;

	// status line
	Status statusCode;
	std::string statusText;

	// headers
	HeaderMap headers;

	// body
	std::string body;

	Response() :
		statusCode(STATUS_OK), statusText(HttpUtils::getStatusText(STATUS_OK))
	{}

	explicit Response(Status code) :
		statusCode(code), statusText(HttpUtils::getStatusText(code))
	{}

	/**
	 * @brief 현재 시각을 RFC 7231 IMF-fixdate 형식("Sun, 06 Nov 1994 08:49:37 GMT")으로 반환
	 */
	static std::string httpDate()
	{
		std::time_t now = std::time(NULL);
		std::tm tmResult;
		gmtime_r(&now, &tmResult);
		char buf[30];
		std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tmResult);
		return std::string(buf);
	}

	std::string toString(bool shouldClose) const
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
		oss << "\r\n" << body;
		return oss.str();
	}
};

#endif