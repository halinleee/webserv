#!/usr/bin/env python3
# RFC3875 6.3.1: header block + blank line + zero-length body is valid
import sys

sys.stdout.write("Content-Type: text/plain\r\nContent-Length: 0\r\n\r\n")
