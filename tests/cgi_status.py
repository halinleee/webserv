#!/usr/bin/env python3
# RFC3875 6.3.3 Status header => server should map this to the HTTP status line
import os
import sys

path = os.environ.get("SCRIPT_NAME", "")
body = ("Not Found: %s" % path).encode("utf-8")

headers = "Status: 404 Not Found\r\n" \
          "Content-Type: text/plain\r\n" \
          "Content-Length: %d\r\n" \
          "\r\n" % len(body)

sys.stdout.write(headers)
sys.stdout.flush()
sys.stdout.buffer.write(body)
