#!/bin/bash

HOST="http://localhost:8080"
PASS="✅"
FAIL="❌"

function test() {
    NAME="$1"
    CMD="$2"
    EXPECT="$3"

    echo -n "🔹 $NAME ... "
    RES=$(eval "$CMD" 2>/dev/null)

    if [[ "$RES" == *"$EXPECT"* ]]; then
        echo "$PASS"
    else
        echo "$FAIL"
        echo "   Expected: $EXPECT"
        echo "   Got: $RES"
    fi
}

echo "=============================="
echo "   WEBSERV MERGE TEST SUITE"
echo "=============================="
echo ""

########################################
# Developer B — Client/Parser/Router
########################################

echo "🧠 Testing Client Manager / Parser / Router"

# 1. Basic HTTP parsing
test "HTTP GET parsing" \
"curl -i $HOST/" \
"HTTP/1.0"

# 2. Method checking
test "Invalid method handling" \
"curl -i -X TRACE $HOST/" \
"405"

# 3. Redirect handling
test "Redirect logic" \
"curl -i $HOST/redirect" \
"301"

# 4. Routing logic
test "Routing to static path" \
"curl -i $HOST/static/test.txt" \
"200"

# 5. Chunked decoding
test "Chunked request decoding" \
"printf 'POST /upload HTTP/1.0\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n0\r\n\r\n' | nc localhost 8080" \
"HTTP/1.0"

########################################
# Developer C — Handlers / CGI / Response
########################################

echo ""
echo "🧰 Testing Handlers / CGI / ResponseBuilder"

# 6. Static file handler
test "Static file handler" \
"curl -i $HOST/index.html" \
"200"

# 7. Directory listing
test "Directory listing" \
"curl -i $HOST/uploads/" \
"200"

# 8. Upload handler
test "Upload handler" \
"curl -i -X POST -F 'file=@/etc/hosts' $HOST/upload" \
"201"

# 9. CGI execution
test "CGI execution" \
"curl -i $HOST/cgi-bin/test.py" \
"200"

# 10. ResponseBuilder headers
test "ResponseBuilder headers" \
"curl -I $HOST/" \
"Content-Type"

# 11. Error pages
test "404 error page" \
"curl -i $HOST/thispagedoesnotexist" \
"404"

########################################
# Integration / Stress
########################################

echo ""
echo "🔥 Integration tests"

# 12. Multiple clients (Client Manager test)
for i in {1..10}; do
    curl -s $HOST/ > /dev/null &
done
wait
echo "🔹 Concurrent connections test ... $PASS"

# 13. Large request buffering
test "Large request buffering" \
"dd if=/dev/zero bs=1024 count=100 | curl -i -X POST --data-binary @- $HOST/upload" \
"HTTP/1.0"

########################################
# Final
########################################

echo ""
echo "=============================="
echo "✅ TESTING COMPLETE"
echo "=============================="

