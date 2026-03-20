#ifndef TYPE_HPP
# define TYPE_HPP

#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>

class Socket;

/**
 * @brief recv해서 나온 Http 요청을 저장하는 <char> 백터
 * 
 * client객체 안에 가지고 있음
 */
typedef std::vector<char> CharVec;

/**
 * @brief 파이프 FD와 클라이언트 FD 매핑해서 가지고 있는 <int, int> 맵
 * 
 * key = PipeFd, value = ClientFd
 */
typedef std::map<int, int> IntMap;

/**
 * @brief CGI구동에 필요한 프로세스의 기본적인 환경변수를 가지고 있는 <string, string> 맵
 * 
 * 
 * PATH=asdf/asdf 일때 key = PATH, value = asdf/asdf
 */
typedef std::map<std::string, std::string> EnvMap;


#endif