#!/usr/bin/env python3
"""
에러 응답 직후 연결 종료 시 half-close(drain) 동작 검증 테스트.

검증 대상: Server::drainSocket() / Server::clientResponse() (src/server/Server.cpp)

배경:
 client_max_body_size(webserv.conf: 1000000)를 초과하는 Content-Length로 요청하면
 RequestParser가 헤더 파싱 단계에서 곧바로 413을 확정하고 연결을 닫는다(FAIL_CLOSE).
 이때 클라이언트가 이미 보낸 잔여 body 데이터가 서버의 recv 버퍼에 남아있는 상태로
 close()를 호출하면, 커널이 정상 FIN 대신 TCP RST를 보내 방금 쓴 413 응답이
 클라이언트에 전달되지 못하거나 잘릴 수 있다.
 drainSocket()은 close() 직전에 그 잔여 데이터를 읽어 버려서 RST 대신 정상 FIN이
 나가도록 보장해야 한다.

검증 방법:
 1) Content-Length를 한도보다 크게 선언한 요청 헤더 뒤에, 실제로는 선언한 값보다
    훨씬 적은(그러나 서버가 413을 감지하고 응답을 다 보낼 때까지 recv 버퍼에
    남아있을 만큼의) body 바이트를 붙여 보낸다.
 2) 413 상태줄과 Content-Length만큼의 바디를 끝까지 정상 수신하는지 확인한다
    (RST가 나면 도중에 ConnectionResetError 또는 EOF로 끊긴다).
 3) 응답을 다 받은 뒤 소켓에서 한 번 더 recv()했을 때 RST(ConnectionResetError)가
    아니라 정상 EOF(빈 바이트열)가 오는지 확인한다.

사용법:
    ./Webserver webserv.conf   # 서버 기동 후
    python3 tests/close_drain_test.py [host] [port]
"""

import socket
import sys

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

OVER_LIMIT_CONTENT_LENGTH = 2000000  # webserv.conf client_max_body_size(1000000)보다 큼
LEFTOVER_BODY_BYTES = 20000  # 서버가 413을 감지/응답할 때 recv 버퍼에 남아있을 잔여 바이트


def run_once():
    req = (
        "POST / HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Connection: close\r\n"
        "\r\n" % (HOST, PORT, OVER_LIMIT_CONTENT_LENGTH)
    ).encode("latin-1")
    # 선언한 Content-Length(2_000_000)만큼 실제로 다 보내지는 않는다.
    # 헤더가 파싱되고 413이 확정될 때까지 recv 버퍼에 남아있을 잔여 바이트만 붙인다.
    req += b"A" * LEFTOVER_BODY_BYTES

    s = socket.create_connection((HOST, PORT), timeout=5)
    s.settimeout(5)
    try:
        s.sendall(req)

        buf = b""
        try:
            while b"\r\n\r\n" not in buf:
                chunk = s.recv(4096)
                if not chunk:
                    return False, "헤더 수신 도중 EOF (%r)" % buf[:80]
                buf += chunk
        except ConnectionResetError:
            return False, "헤더 수신 중 ConnectionResetError(RST) - 413 응답 자체가 유실됨"

        head, _, body = buf.partition(b"\r\n\r\n")
        head_text = head.decode("latin-1")
        status_line = head_text.split("\r\n", 1)[0]
        if "413" not in status_line:
            return False, "예상한 413이 아님: %r" % status_line

        headers = {}
        for line in head_text.split("\r\n")[1:]:
            if ":" in line:
                k, _, v = line.partition(":")
                headers[k.strip().lower()] = v.strip()
        clen = int(headers.get("content-length", "0"))

        try:
            while len(body) < clen:
                chunk = s.recv(4096)
                if not chunk:
                    return False, "바디 수신 도중 EOF (received=%d/%d)" % (len(body), clen)
                body += chunk
        except ConnectionResetError:
            return False, "바디 수신 중 ConnectionResetError(RST) - 413 응답 바디가 잘림"

        if len(body) != clen:
            return False, "바디 길이 불일치 (received=%d, expected=%d)" % (len(body), clen)

        # 응답을 온전히 다 받은 뒤: half-close가 지켜졌다면 이제 recv()는
        # RST(ConnectionResetError)가 아니라 정상 EOF(빈 바이트열)를 반환해야 한다.
        try:
            extra = s.recv(4096)
        except ConnectionResetError:
            return False, "응답 완전 수신 후 소켓 종료 시 ConnectionResetError(RST) 발생"

        if extra != b"":
            return False, "정상 종료가 아닌 예상 밖 데이터 수신: %r" % extra[:80]

        return True, "413 응답(%d bytes)을 온전히 수신했고, 연결도 RST 없이 정상 종료됨" % clen
    finally:
        try:
            s.close()
        except OSError:
            pass


def main():
    ROUNDS = 5
    failures = 0
    for i in range(1, ROUNDS + 1):
        try:
            ok, detail = run_once()
        except Exception as e:  # noqa: BLE001
            ok, detail = False, "예외: %s" % e
        mark = "PASS" if ok else "FAIL"
        print("[%s] round %d/%d: %s" % (mark, i, ROUNDS, detail))
        if not ok:
            failures += 1

    print("총 %d회 중 실패 %d회" % (ROUNDS, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
