#include <iostream>
#include <string>
#include <cstdlib>

int main() 
{
    // 1. 서버가 넘겨준 환경변수에서 CONTENT_LENGTH 읽기
    char* contentLengthEnv = std::getenv("CONTENT_LENGTH");
    int contentLength = 0;
    
    if (contentLengthEnv != NULL)
        contentLength = std::atoi(contentLengthEnv);

    // 2. 표준 입력(stdin)으로부터 CONTENT_LENGTH 만큼 읽기
    // 서버가 여러 번 나누어 보내더라도, cin.read()는 그만큼 다 들어올 때까지 기다립니다.
    std::string body;
    if (contentLength > 0) 
    {
        char* buffer = new char[contentLength];
        std::cin.read(buffer, contentLength);
        body.assign(buffer, contentLength);
        delete[] buffer;
    }

    // 3. 표준 출력(stdout)으로 결과 응답하기 (서버의 OutPipe로 들어감)
    // 반드시 HTTP 헤더(Content-Type 등)를 먼저 출력하고 빈 줄(\r\n\r\n)을 넣어야 합니다.
    std::cout << "Content-Type: text/html; charset=utf-8\r\n\r\n";
    std::cout << "<html><body>\n";
    std::cout << "<h1>CGI Success!</h1>\n";
    std::cout << "<p>Received " << contentLength << " bytes from WebServer.</p>\n";
    std::cout << "<p>Body Data: " << body << "</p>\n";
    std::cout << "</body></html>\n";

    // 4. 정상 종료되면 파이프의 쓰기 끝단이 닫히며 서버 측에 EOF(length == 0)가 전달됨
    return 0;
}