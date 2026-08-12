#include "MultipartParser.hpp"
#include "HttpUtils.hpp"

#include <cctype>

namespace
{
	std::string extractDispositionValue(const std::string& line, const std::string& key)
	{
		std::string token = key + "=\"";
		size_t pos = 0;

		while (true)
		{
			pos = line.find(token, pos);
			if (pos == std::string::npos)
				return "";
			if (pos > 0 && std::isalpha(static_cast<unsigned char>(line[pos - 1])))
			{
				pos += token.size();
				continue;
			}
			break;
		}

		size_t start = pos + token.size();
		size_t end = line.find('"', start);
		if (end == std::string::npos)
			return "";
		return line.substr(start, end - start);
	}

	bool parsePartHeaders(const std::string& headers, MultipartPart& part)
	{
		size_t start = 0;
		bool hasContentDisposition = false;

		while (start <= headers.size())
		{
			size_t end = headers.find("\r\n", start);
			std::string line = (end == std::string::npos) ? headers.substr(start) : headers.substr(start, end - start);

			std::string lower = HttpUtils::toLower(line);

			if (lower.compare(0, 20, "content-disposition:") == 0)
			{
				size_t colon = line.find(':');
				std::string value = line.substr(colon + 1);
				size_t valStart = value.find_first_not_of(" \t");
				std::string dispositionType = (valStart == std::string::npos) ? "" : value.substr(valStart);
				size_t semi = dispositionType.find(';');
				if (semi != std::string::npos)
					dispositionType = dispositionType.substr(0, semi);
				size_t typeEnd = dispositionType.find_last_not_of(" \t");
				dispositionType = (typeEnd == std::string::npos) ? "" : dispositionType.substr(0, typeEnd + 1);
				if (HttpUtils::toLower(dispositionType) != "form-data")
					return false;

				part.name = extractDispositionValue(line, "name");
				if (part.name.empty())
					return false;
				part.filename = extractDispositionValue(line, "filename");
				hasContentDisposition = true;
			}
			else if (lower.compare(0, 13, "content-type:") == 0)
			{
				size_t colon = line.find(':');
				std::string value = line.substr(colon + 1);
				size_t valStart = value.find_first_not_of(" \t");
				part.contentType = (valStart == std::string::npos) ? "" : value.substr(valStart);
			}

			if (end == std::string::npos)
				break;
			start = end + 2;
		}

		if (!hasContentDisposition)
			return false;

		if (part.contentType.empty())
			part.contentType = "text/plain; charset=US-ASCII";

		return true;
	}
}

bool MultipartParser::parse(const std::string& body, const std::string& boundary, std::vector<MultipartPart>& outParts)
{
	if (boundary.empty())
		return false;

	std::string delim = "--" + boundary;
	size_t pos = body.find(delim);
	if (pos == std::string::npos)
		return false;
	pos += delim.size();

	while (true)
	{
		if (pos + 2 > body.size())
			return false;

		if (body.compare(pos, 2, "--") == 0)
			return true;

		if (body.compare(pos, 2, "\r\n") != 0)
			return false;
		pos += 2;

		size_t headerEnd = body.find("\r\n\r\n", pos);
		if (headerEnd == std::string::npos)
			return false;

		std::string headers = body.substr(pos, headerEnd - pos);
		size_t dataStart = headerEnd + 4;

		size_t nextDelim = body.find(delim, dataStart);
		if (nextDelim == std::string::npos)
			return false;

		size_t dataEnd = nextDelim;
		if (dataEnd < dataStart + 2 || body.compare(dataEnd - 2, 2, "\r\n") != 0)
			return false;
		dataEnd -= 2;

		MultipartPart part;
		part.data = body.substr(dataStart, dataEnd - dataStart);
		if (!parsePartHeaders(headers, part))
			return false;
		outParts.push_back(part);

		pos = nextDelim + delim.size();
	}
}

