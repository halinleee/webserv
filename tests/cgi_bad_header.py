#!/usr/bin/env python3
# Negative case: header line missing ':' violates RFC3875 6.1 field-name ':' field-value syntax.
# Used to verify the server rejects/500s malformed CGI output instead of forwarding it as-is.
import sys

sys.stdout.write("ThisHeaderHasNoColon\r\nContent-Type text/plain\r\n\r\nbroken\r\n")
