#!/usr/bin/env python3
import os
import sys

# Check if environment variables should be displayed
query_string = os.environ.get('QUERY_STRING', '')

print("Content-Type: text/html\n")

if 'env' in query_string:
    # Display environment variables for testing
    print("<h1>CGI Environment</h1>")
    print("<pre>")
    print(f"REQUEST_METHOD={os.environ.get('REQUEST_METHOD', 'N/A')}")
    print(f"QUERY_STRING={query_string}")
    print(f"CONTENT_TYPE={os.environ.get('CONTENT_TYPE', 'N/A')}")
    print(f"CONTENT_LENGTH={os.environ.get('CONTENT_LENGTH', 'N/A')}")
    print(f"SCRIPT_NAME={os.environ.get('SCRIPT_NAME', 'N/A')}")
    print(f"PATH_INFO={os.environ.get('PATH_INFO', 'N/A')}")
    print("</pre>")
else:
    # Normal output
    print("Hello CGI")
