#ifndef Handler_HPP
# define Handler_HPP

#include "RouteResult.hpp"
#include "Request.hpp"
#include "Response.hpp"

#include <string>
#include <map>

class Handler
{
	private:
		static Response handleGet(const RouteResult& route, const std::string& reqPath);
		static Response handlePost(const RouteResult& route, const Request& req);
		static Response handleDelete(const RouteResult& route);

		static Response buildAutoIndexPage(const std::string& dirPath, const std::string& reqPath);
		static Response checkErrno(int err);

	public:
		static Response serve(const RouteResult& route, const Request& req);

		/**
		 * @brief 상태 코드에 맞는 에러 페이지 body를 채운 Response를 만드는 함수
		 *
		 * errorPages에 해당 상태 코드의 커스텀 페이지 경로가 있으면 그 파일을 읽어 body로 쓰고,
		 * 없거나 읽기에 실패하면 webserv 자체 기본 에러 페이지(www/error/default.html)로 대체한다.
		 * @param code 응답에 사용할 상태 코드
		 * @param errorPages ServerConfig::getErrorPages()가 반환하는 상태 코드 -> 커스텀 페이지 경로 맵
		 */
		static Response buildErrorPage(Status code, const std::map<size_t, std::string>& errorPages);
};

#endif
