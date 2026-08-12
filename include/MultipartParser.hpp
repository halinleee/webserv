#ifndef MULTIPARTPARSER_HPP
# define MULTIPARTPARSER_HPP

#include <string>
#include <vector>

struct MultipartPart
{
	std::string name;
	std::string filename;
	std::string contentType;
	std::string data;
};

class MultipartParser
{
	public:
		static bool parse(const std::string& body, const std::string& boundary, std::vector<MultipartPart>& outParts);
};

#endif

