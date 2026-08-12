#ifndef HTTPUTILS_HPP
#define HTTPUTILS_HPP

#include "type.hpp"

#include <string>

namespace HttpUtils
{
	const size_t npos = static_cast<size_t>(-1);

	void consumeLeadingCRLF(CharDq& buf, size_t maxBlankLines);

	size_t findCRLF(const CharDq& buf);

	size_t findCRLFCRLF(const CharDq& buf);

	size_t findBareLF(const CharDq& buf);

	std::string extractLine(CharDq& buf, size_t end_pos, size_t end_size);

	bool hasCR(const std::string& line);

	bool isHex(const char c);

	bool hasConsecutiveSlashes(const std::string& str);

	bool hasDotSegments(const std::string& str);

	int hexToInt(const char c);

	bool isTchar(unsigned char c);

	bool isVcharSpTab(unsigned char c);

	std::string getStatusText(Status code);

	std::string getMethodName(HttpMethod method);

	std::string getMimeType(const std::string& path);

	std::string toLower(const std::string& s);

	std::string htmlEscape(const std::string& s);

	std::string urlEncode(const std::string& s);

	std::string joinPath(const std::string& base, const std::string& tail);
}

#endif