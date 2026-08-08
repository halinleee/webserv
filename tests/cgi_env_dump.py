#!/usr/bin/env python3
# Dump CGI meta-variables (RFC3875 4.1) to verify Cgi::envAppend on the server side.
import os
import sys

names = [
    "REQUEST_METHOD", "SCRIPT_NAME", "QUERY_STRING", "SERVER_PROTOCOL",
    "GATEWAY_INTERFACE", "SERVER_NAME", "SERVER_PORT", "REMOTE_ADDR",
    "CONTENT_LENGTH", "CONTENT_TYPE",
]
body = "\r\n".join("%s=%s" % (name, os.environ.get(name, "")) for name in names).encode("utf-8")

headers = "Content-Type: text/plain\r\nContent-Length: %d\r\n\r\n" % len(body)

sys.stdout.write(headers)
sys.stdout.flush()
sys.stdout.buffer.write(body)
