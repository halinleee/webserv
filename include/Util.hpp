# ifndef UTIL_HPP
# define UTIL_HPP

#include <string>
#include <vector>
#include <sstream>

bool isBlankLine(const std::string &line);
int countIndent(const std::string &line);
bool removeIndent (std::string &value, char delim);
std::vector<std::string> ftSplit(const std::string &line, char delim);
bool isNumber(const std::string &s);
bool toInt (const std::string &s, unsigned int &value);
bool isValidUriPath(std::string &path);
bool isValidPrefix(std::string &path);


# endif