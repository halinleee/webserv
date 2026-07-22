#!/usr/bin/env python3
# RFC3875 6.2.2 local redirect: Location starts with "/", no other header/body allowed
import sys

sys.stdout.write("Location: /index.html\r\n\r\n")
