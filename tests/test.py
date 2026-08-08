#!/usr/bin/env python3
import datetime
import sys
from email.utils import formatdate

# 1. 현재 시간 구하기
now = datetime.datetime.now()
custom_time_str = now.strftime("%Y-%m-%d %H:%M:%S")

# HTTP 표준(RFC 2822)에 맞는 Date 헤더용 시간 (GMT 기준)
standard_date_str = formatdate(timeval=None, localtime=False, usegmt=True)

# 2. 본문(Body)을 먼저 완성한다.
# Content-Length는 이 본문의 실제 바이트 길이로부터 계산해야 한다.
# (response 빌더가 아직 없으므로, CGI 스크립트가 직접 올바른 HTTP 응답을 만들어야 함)
body_lines = [
    "<!DOCTYPE html>",
    "<html>",
    "<head>",
    "<title>CGI Time Header Test</title>",
    "</head>",
    "<body>",
    "<h1>CGI 스크립트 실행 성공</h1>",
    "<p>현재 시간 <strong>{0}</strong>이 HTTP 응답 헤더에 정상적으로 삽입되었습니다.</p>".format(custom_time_str),
    "<p>브라우저 개발자 도구(F12)의 '네트워크(Network)' 탭에서 응답 헤더(Response Headers)를 확인해 보세요.</p>",
    "</body>",
    "</html>",
]
body = "\r\n".join(body_lines)
body_bytes = body.encode("utf-8")

# 3. 헤더를 본문 길이를 기준으로 구성한다.
# 헤더와 본문 사이에는 빈 줄(\r\n\r\n)이 정확히 한 번만 있어야 한다.
headers = [
    "HTTP/1.1 200 OK",
    "Content-type: text/html; charset=utf-8",
    "Content-Length: {0}".format(len(body_bytes)),
    "Date: {0}".format(standard_date_str),
    "X-Current-Time: {0}".format(custom_time_str),
]
header_block = "\r\n".join(headers) + "\r\n\r\n"

# 4. \r\n 임의 삽입/print() 개행 혼용으로 헤더 경계가 깨지는 것을 막기 위해
# 텍스트 헤더와 바이너리 본문을 각각 명시적으로 한 번에 출력한다.
sys.stdout.write(header_block)
sys.stdout.flush()
sys.stdout.buffer.write(body_bytes)
sys.stdout.flush()
