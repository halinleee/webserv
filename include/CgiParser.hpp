#ifndef CGIPARSER_HPP
#define CGIPARSER_HPP

#include "HttpUtils.hpp"
#include "Response.hpp"
#include "ConfigParseUtils.hpp"

#include <iostream>

/**
 * CGI 프로세스의 stdout 출력(cgiRawOutput) 크기 제한 상수
 * 폭주(무한 루프) 스크립트로부터 서버 메모리를 보호하기 위한 상한
 */
const size_t MAX_CGI_OUTPUT_LENGTH = 10 * 1024 * 1024;

class CgiParser
{
	public:
		Response parseCgiOutput(const std::string& cgiOutput);
};

#endif

/*

HTTP/1.1 200 OK\r\n

Content-type: text/html; charset=utf-8\r\n
Content-Length: 403\r\n
Date: Sun, 28 Jun 2026 10:25:53 GMT\r\n
X-Current-Time: 2026-06-28 19:25:53\r\n
\r\n

<!DOCTYPE html>\r\n
<html>\r\n
... (body, 한글은 UTF-8로 깨져 보이지만 실제로는 정상)
</html>

*/