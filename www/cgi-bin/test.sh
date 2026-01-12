#!/usr/bin/bash

# CGI script must output headers first
echo "Content-Type: text/html"
echo ""

# Then output the body
echo "<!DOCTYPE html>"
echo "<html>"
echo "<head><title>CGI Test</title></head>"
echo "<body>"
echo "<h1>CGI Script Works!</h1>"
echo "<p>Request Method: $REQUEST_METHOD</p>"
echo "<p>Script Name: $SCRIPT_NAME</p>"
echo "<p>Query String: $QUERY_STRING</p>"
echo "<p>Server Name: $SERVER_NAME</p>"
echo "<p>Server Port: $SERVER_PORT</p>"
echo "</body>"
echo "</html>"
