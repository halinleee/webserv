#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include "type.hpp"

typedef struct
{
	// request line
	HttpMethod method;
	std::string path;
	std::string query;

	// headers

	// body
	
} Request;

// 생성자 필요시 정의
// method 초기값은 unknown

#endif

/*

- 일반 헤더는 하나만 유지 (마지막에 들어온 값으로)
- `Set-Cookie`나 `Cookie`, `X-Forwarded-For`와 같은 헤더는 여러 개 허용
- `Content-Length`나 `Host`는 무조건 한 개만 들어와야 함.
- `Content-Length`나 `Transfer-Encoding`이 같이 오면 유효하지 않은 요청

*/