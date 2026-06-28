#ifndef CGIPARSER_HPP
#define CGIPARSER_HPP

#include "HttpUtils.hpp"
#include "Response.hpp"
#include "Util.hpp"
#include <iostream>

class CgiParser
{
	private:
		bool cgiParseKeyValue(const std::string& line, std::string& key, std::string& value);
		bool splitHeaderBody(const std::string& stdout, Response& response, std::string& headers);
		bool parseStatusLine(const std::string& lines, Response& response);
		bool findCRLF(std::string headersLine);

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

/*
CGI가 잘못된 헤더를 출력한 경우 어떻게 처리할지 생각하기

1. CGI stdout 전체 읽기 [v]
2. 헤더 블록과 Body 분리 [v]
3. Body 저장 [v]
4. 헤더를 한 줄씩 읽기 [v] (첫줄 파싱때 status code 예외처리 생각)
5. 각 헤더를 key/value로 분리
6. Status면 statusCode/statusText 설정
7. 그 외 헤더는 Response 헤더에 저장
8. Status가 없으면 200 OK 적용
9. Content-Length 계산 -> response.body.size()
10. 최종 HTTP Response 생성
*/