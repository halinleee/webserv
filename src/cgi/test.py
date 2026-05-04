#!/usr/bin/env python3
import datetime
from email.utils import formatdate

# 1. 현재 시간 구하기
# 커스텀 헤더용으로 보기 쉽게 포맷팅한 시간
now = datetime.datetime.now()
custom_time_str = now.strftime("%Y-%m-%d %H:%M:%S")

# HTTP 표준(RFC 2822)에 맞는 Date 헤더용 시간 (GMT 기준)
standard_date_str = formatdate(timeval=None, localtime=False, usegmt=True)

# 2. HTTP 헤더 출력
# Content-type은 필수 헤더입니다.
print("HTTP/1.1 200 OK")
print("Content-type: text/html; charset=utf-8\r\n")

# 표준 Date 헤더 출력
print(f"Date: {standard_date_str}")

# 사용자 정의 헤더(X-Custom-Time)에 현재 시간 출력
print(f"X-Current-Time: {custom_time_str}")

# ★ 중요: 헤더와 본문(Body)을 구분하기 위해 반드시 빈 줄을 하나 출력해야 합니다.
print() 

# 3. HTTP 본문(HTML) 출력
print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("<title>CGI Time Header Test</title>")
print("</head>")
print("<body>")
print("<h1>CGI 스크립트 실행 성공</h1>")
print(f"<p>현재 시간 <strong>{custom_time_str}</strong>이 HTTP 응답 헤더에 정상적으로 삽입되었습니다.</p>")
print("<p>브라우저 개발자 도구(F12)의 '네트워크(Network)' 탭에서 응답 헤더(Response Headers)를 확인해 보세요.</p>")
print("</body>")
print("</html>")