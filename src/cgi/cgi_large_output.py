#!/usr/bin/env python3
# Large body to exercise multi-chunk pipe read/write on the server side.
import sys

body = (b"0123456789" * 100 + b"\r\n") * 1000  # ~1.1MB

headers = "Content-Type: text/plain\r\nContent-Length: %d\r\n\r\n" % len(body)

sys.stdout.write(headers)
sys.stdout.flush()
sys.stdout.buffer.write(body)
