#ifndef Handler_HPP
# define Handler_HPP

#include "RouteResult.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <string>

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
};

#endif
