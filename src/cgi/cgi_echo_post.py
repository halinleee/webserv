#!/usr/bin/env python3
# RFC3875 4.2: read exactly CONTENT_LENGTH bytes from stdin, not until EOF.
import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.buffer.read(length)

headers = "Content-Type: %s\r\nContent-Length: %d\r\n\r\n" % (
    os.environ.get("CONTENT_TYPE", "text/plain"), len(body)
)

sys.stdout.write(headers)
sys.stdout.flush()
sys.stdout.buffer.write(body)
