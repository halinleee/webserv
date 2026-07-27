#!/usr/bin/env python3
# RFC3875 6.3.1 default response (no Status header => 200 implied by server)
import os
import sys

method = os.environ.get("REQUEST_METHOD", "")
script = os.environ.get("SCRIPT_NAME", "")
query = os.environ.get("QUERY_STRING", "")

body = (
    "<html><body>"
    "<h1>CGI OK</h1>"
    "<p>METHOD: {0}</p>"
    "<p>SCRIPT_NAME: {1}</p>"
    "<p>QUERY_STRING: {2}</p>"
    "</body></html>"
).format(method, script, query).encode("utf-8")

headers = "Content-Type: text/html\r\n" \
          "Content-Length: %d\r\n" \
          "\r\n" % len(body)

sys.stdout.write(headers)
sys.stdout.flush()
sys.stdout.buffer.write(body)
