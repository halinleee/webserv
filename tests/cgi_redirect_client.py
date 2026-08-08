#!/usr/bin/env python3
# RFC3875 6.2.3 client redirect: absolute Location + Status, body optional
import os
import sys

host = os.environ.get("SERVER_NAME", "localhost")
body = b"redirecting...\n"

headers = "Status: 302 Found\r\n" \
          "Location: http://%s/\r\n" \
          "Content-Type: text/plain\r\n" \
          "Content-Length: %d\r\n" \
          "\r\n" % (host, len(body))

sys.stdout.write(headers)
sys.stdout.flush()
sys.stdout.buffer.write(body)
