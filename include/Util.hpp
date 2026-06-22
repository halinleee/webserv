# ifndef UTIL_HPP
# define UTIL_HPP

#include <string>
#include <vector>

bool isValidFileName(const std::string &file);
bool isValidNormalizePath(std::string& path);
void removeIndent (std::string& value, char delim);
int countIndent(const std::string& line);
bool isBlankLine(const std::string& line);
std::vector<std::string> ftSplit(const std::string& line, char delim);
bool isNumber(const std::string& s);
bool toInt (const std::string& s, size_t& value);

# endif