#ifndef TYPE_HPP
# define TYPE_HPP

#include <map>
#include <vector>
#include <algorithm>
#include <iostream>

class Socket;

/**
 * @brief CGI구동에 필요한 프로세스의 기본적인 환경변수를 가지고 있는 <string, string> 맵
 * @details PATH=asdf/asdf 일때 key = PATH, value = asdf/asdf
 */
typedef std::map<std::string, std::string> EnvMap;


#endif