#ifndef TYPE_HPP
# define TYPE_HPP

#include <map>
#include <vector>
#include <algorithm>
#include <iostream>

/**
 * @brief 문자열(std::string)을 키와 값으로 가지는 맵 (주로 환경변수나 HTTP 헤더 저장용)
 */
typedef std::map<std::string, std::string> EnvMap;


#endif