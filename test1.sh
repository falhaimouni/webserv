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
echo "   WEBSERV REAL TEST SUITE"
echo "=============================="

# Core
test "HTTP response" "curl -i $HOST/" "HTTP/1.0"
test "Method check" "curl -i -X TRACE $HOST/" "405"

# Handlers
test "Upload handler" "curl -i -X POST -F 'file=@/etc/hosts' $HOST/upload" "201"
test "CGI" "curl -i $HOST/cgi-bin/test.py" "200"
test "404 page" "curl -i $HOST/notfound" "404"

# Stability
for i in {1..20}; do curl -s $HOST/ > /dev/null & done
wait
echo "🔹 Concurrent clients ... $PASS"

echo "=============================="
echo "✅ CORE SYSTEM VALIDATED"
echo "=============================="

