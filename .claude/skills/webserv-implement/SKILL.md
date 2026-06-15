---
name: webserv-implement
description: >
  webserv C++ HTTP 웹서버 기능 구현 스킬. HTTP 파싱, Config 파싱, 응답 빌더, 정적 파일 서빙, CGI 연동 등
  webserv의 모든 기능 구현 작업에 사용한다. "구현해줘", "만들어줘", "추가해줘", "HTTP 파싱", "Config",
  "응답 빌더", "GET/POST/DELETE 처리", "에러 페이지", "리다이렉션" 요청 시 반드시 이 스킬을 사용할 것.
---

# webserv 구현 스킬

## 환경 정보

- **빌드**: `make -C /home/seungsikchoi/code/Webserver`
- **표준**: C++98 (컴파일러 플래그 `-std=c++98 -Wall -Wextra -Werror`)
- **실행**: `./Webserver {port}` 또는 `./Webserver {config_file}`
- **소스 루트**: `/home/seungsikchoi/code/Webserver/`

## C++98 제약 — 핵심 규칙

| 금지 | 대체 |
|------|------|
| `auto` | 명시적 타입 선언 |
| 람다 | 함수 포인터 또는 함수 객체 |
| `nullptr` | `NULL` 또는 `0` |
| `std::shared_ptr` | 원시 포인터 + 명시적 delete |
| range-for | 이터레이터 기반 for |
| `std::to_string` | `std::ostringstream` |
| `std::stoi` | `std::atoi` 또는 `std::strtol` |

## 코드 패턴 — 기존 코드베이스 컨벤션

**클래스 구조** (기존 `Server`, `Client`, `Socket` 참조):
```cpp
// include/ClassName.hpp
#ifndef CLASSNAME_HPP
# define CLASSNAME_HPP

#include "main.hpp"

/**
 * @brief 클래스 한 줄 설명
 */
class ClassName
{
    private:
        /**
         * @var memberName
         * @brief 멤버 변수 설명
         */
        Type memberName;

    public:
        ClassName();
        ~ClassName();

        /**
         * @brief 함수 설명
         * @param param 파라미터 설명
         * @return 반환값 설명
         */
        ReturnType functionName(Type param);
};

#endif
```

**에러 처리 패턴**:
```cpp
// STATUS_OK = true(1), STATUS_ERROR = false(0)
bool Server::someFunction() {
    if (condition fails)
        return (false);  // 괄호 스타일 유지
    return (true);
}
```

**환경변수 맵 순회** (C++98):
```cpp
for (EnvMap::iterator it = env.begin(); it != env.end(); ++it) {
    std::string key = it->first;
    std::string val = it->second;
}
```

## HTTP/1.1 구현 가이드

### 요청 파싱 순서
1. **Request-Line**: `METHOD SP Request-URI SP HTTP-Version CRLF`
2. **헤더**: `field-name ":" SP field-value CRLF` (대소문자 비민감)
3. **빈 줄**: `CRLF`
4. **바디**: `Content-Length` 기준으로 읽기

### 필수 구현 상태 코드
- `200 OK`, `201 Created`, `204 No Content`
- `301 Moved Permanently`, `302 Found`
- `400 Bad Request`, `403 Forbidden`, `404 Not Found`
- `405 Method Not Allowed`, `413 Payload Too Large`
- `500 Internal Server Error`, `505 HTTP Version Not Supported`

### 42 subject 필수 메서드
- `GET`, `POST`, `DELETE`

### 응답 빌더 패턴
```
HTTP/1.1 {status_code} {reason}\r\n
{header-name}: {value}\r\n
...
\r\n
{body}
```

## Makefile 모듈 추가 방법

`src/http/` 모듈 추가 예시:
```makefile
HTTP_DIR  = ./src/http
HTTP_SRC  = HttpRequest.cpp HttpResponse.cpp
HTTP_OBJ  = $(addprefix $(OBJS_DIR)/, $(HTTP_SRC:.cpp=.o))
# OBJS에 $(HTTP_OBJ) 추가
# vpath에 $(HTTP_DIR) 추가
```

## 구현 완료 시 체크리스트

- [ ] `make re` 빌드 성공 (경고 0개)
- [ ] 헤더 파일에 Doxygen 주석 완성
- [ ] 소멸자에서 모든 `new`/파이프 FD 해제
- [ ] `_workspace/impl_{module}_done.md`에 변경 파일 목록 저장
