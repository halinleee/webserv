#!/usr/bin/env python3
"""
Connection 헤더 기반 keep-alive/close 판단 로직 검증 테스트.

검증 대상: Client::onReceive() (src/server/Client.cpp)
 - Connection 헤더 없음        -> keep-alive 유지
 - Connection: close           -> 응답 후 서버가 연결 종료
 - Connection: Close (대소문자) -> 종료
 - Connection: keep-alive, close (콤마 토큰 리스트) -> 종료
 - Connection: keep-alive      -> 유지

사용법:
    ./Webserver <conf>   # 서버 기동 후
    python3 tests/connection_keepalive_test.py [host] [port]
"""

import socket
import sys

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8081

IDLE_WAIT = 1.5  # 서버가 닫을 시간을 주는 유휴 대기(초)


def recv_response(sock, timeout=5.0):
    """헤더 + Content-Length 만큼의 바디를 읽어 (raw_bytes, headers_dict) 반환."""
    sock.settimeout(timeout)
    buf = b""
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            raise RuntimeError("헤더 수신 도중 EOF: %r" % buf)
        buf += chunk
    head, _, body = buf.partition(b"\r\n\r\n")
    lines = head.decode("latin-1").split("\r\n")
    status = lines[0]
    headers = {}
    for line in lines[1:]:
        if ":" in line:
            k, _, v = line.partition(":")
            headers[k.strip().lower()] = v.strip()
    clen = int(headers.get("content-length", "0"))
    while len(body) < clen:
        chunk = sock.recv(4096)
        if not chunk:
            break
        body += chunk
    return status, headers, body


def peer_closed(sock, wait=IDLE_WAIT):
    """서버가 연결을 닫았으면 True (recv가 0바이트 반환), 열려 있으면 False."""
    sock.settimeout(wait)
    try:
        extra = sock.recv(4096)
    except socket.timeout:
        return False, b""
    return (extra == b""), extra


def build_req(path="/", conn_header=None, host=None):
    host = host or ("%s:%d" % (HOST, PORT))
    req = "GET %s HTTP/1.1\r\nHost: %s\r\n" % (path, host)
    if conn_header is not None:
        req += "Connection: %s\r\n" % conn_header
    req += "\r\n"
    return req.encode("latin-1")


def run_case(name, conn_header, expect_close):
    result = {
        "name": name,
        "conn_header": conn_header,
        "expect_close": expect_close,
        "status": None,
        "resp_connection": None,
        "actual_closed": None,
        "second_request": None,
        "detail": "",
        "passed": False,
    }
    s = socket.create_connection((HOST, PORT), timeout=5)
    try:
        req = build_req(conn_header=conn_header)
        s.sendall(req)
        status, headers, _body = recv_response(s)
        result["status"] = status
        result["resp_connection"] = headers.get("connection")

        closed, extra = peer_closed(s)
        result["actual_closed"] = closed
        if not closed and extra:
            result["detail"] += "유휴 대기 중 예상 밖 추가 데이터: %r; " % extra[:80]

        if not expect_close and not closed:
            # 같은 소켓으로 2번째 요청 -> 응답이 오는지 확인
            try:
                s.sendall(build_req(conn_header=conn_header))
                status2, _h2, _b2 = recv_response(s)
                result["second_request"] = status2
            except Exception as e:  # noqa: BLE001
                result["second_request"] = "FAILED: %s" % e

        ok = (closed == expect_close)
        if expect_close:
            # close 케이스는 응답 헤더에 Connection: close 가 있어야 함
            if result["resp_connection"] != "close":
                ok = False
                result["detail"] += "응답에 'Connection: close' 헤더 없음(값=%r); " % result["resp_connection"]
        else:
            if result["second_request"] is None or str(result["second_request"]).startswith("FAILED"):
                ok = False
                result["detail"] += "재사용 소켓의 2번째 요청 실패; "
        result["passed"] = ok
    finally:
        try:
            s.close()
        except OSError:
            pass
    return result


CASES = [
    ("a. Connection 헤더 없음", None, False),
    ("b. Connection: close", "close", True),
    ("c. Connection: Close (대문자)", "Close", True),
    ("d. Connection: keep-alive, close", "keep-alive, close", True),
    ("e. Connection: keep-alive", "keep-alive", False),
    ("f. Connection:   CLOSE   (공백/전대문자)", "  CLOSE  ", True),
    ("g. Connection: close, keep-alive (순서 반대)", "close, keep-alive", True),
    ("h. Connection: keep-alive, Foo", "keep-alive, Foo", False),
]


def main():
    results = []
    for name, hdr, expect in CASES:
        try:
            r = run_case(name, hdr, expect)
        except Exception as e:  # noqa: BLE001
            r = {
                "name": name, "conn_header": hdr, "expect_close": expect,
                "status": None, "resp_connection": None, "actual_closed": None,
                "second_request": None, "detail": "예외: %s" % e, "passed": False,
            }
        results.append(r)
        mark = "PASS" if r["passed"] else "FAIL"
        print("[%s] %s" % (mark, name))
        print("      요청 Connection: %r" % (r["conn_header"],))
        print("      응답 상태행     : %s" % r["status"])
        print("      응답 Connection : %r" % r["resp_connection"])
        print("      예상 종료=%s / 실제 종료=%s" % (r["expect_close"], r["actual_closed"]))
        if r["second_request"] is not None:
            print("      2차 요청 응답   : %s" % r["second_request"])
        if r["detail"]:
            print("      비고: %s" % r["detail"])
        print()

    failed = [r for r in results if not r["passed"]]
    print("총 %d 케이스 / PASS %d / FAIL %d" % (len(results), len(results) - len(failed), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
