#!/usr/bin/env python3
# ponytail: 고정 호스트/포트, 인자 1개(요청 개수)만 받음 - 옵션 더 필요해지면 argparse로 확장
import socket
import sys

HOST, PORT = "127.0.0.1", 8080
count = int(sys.argv[1]) if len(sys.argv) > 1 else 2

req = b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
payload = req * count

s = socket.create_connection((HOST, PORT), timeout=3)
s.sendall(payload)  # N개 요청을 한 번의 write로 동시에 흘려보냄 (파이프라이닝)

s.settimeout(2)
chunks = []
try:
    while True:
        data = s.recv(65536)
        if not data:
            break
        chunks.append(data)
except socket.timeout:
    pass
s.close()

raw = b"".join(chunks)
responses = raw.count(b"HTTP/1.1")
print(f"보낸 요청 수: {count}")
print(f"받은 응답 수(HTTP/1.1 등장 횟수): {responses}")
print(f"받은 바이트 수: {len(raw)}")
if responses != count:
    print(f"FAIL: {count}개 보냈는데 {responses}개만 응답받음")
else:
    print("OK: 보낸 만큼 응답받음")
