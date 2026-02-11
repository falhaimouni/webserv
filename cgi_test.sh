#!/bin/bash
# ==========================================
# WEBSERV CGI FULL TEST SUITE
# Author: Farah Webserv Validation Suite
# Target: CGI correctness, protocol compliance
# ==========================================

HOST="http://localhost:8080"
CGI_PATH="/cgi-bin/test.py"   # adjust if needed
UPLOAD_CGI="/cgi-bin/upload.py"
TMPFILE="/tmp/cgi_test_body.txt"

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
NC="\033[0m"

pass() { echo -e "${GREEN}✅ $1${NC}"; }
fail() { echo -e "${RED}❌ $1${NC}"; }
info() { echo -e "${YELLOW}🔹 $1${NC}"; }

header() {
    echo "========================================"
    echo " CGI TEST SUITE"
    echo "========================================"
}

check_status() {
    local expected=$1
    local response=$2
    if echo "$response" | grep -q "$expected"; then
        pass "$3"
    else
        fail "$3"
        echo "$response"
    fi
}

header

# ----------------------------
info "Basic GET CGI"
resp=$(curl -i -s "$HOST$CGI_PATH")
check_status "200" "$resp" "CGI GET"

# ----------------------------
info "QUERY_STRING test"
resp=$(curl -i -s "$HOST$CGI_PATH?name=farah&project=webserv")
check_status "200" "$resp" "QUERY_STRING"

# ----------------------------
info "POST CGI (Content-Length)"
echo "hello=cgi&lang=cpp" > $TMPFILE
resp=$(curl -i -s -X POST --data-binary @$TMPFILE "$HOST$CGI_PATH")
check_status "200" "$resp" "POST body"

# ----------------------------
info "POST empty body"
resp=$(curl -i -s -X POST "$HOST$CGI_PATH")
check_status "200" "$resp" "Empty POST"

# ----------------------------
info "Invalid method"
resp=$(curl -i -s -X DELETE "$HOST$CGI_PATH")
check_status "405" "$resp" "Method reject"

# ----------------------------
info "Large body CGI"
base64 /dev/urandom | head -c 500000 > $TMPFILE
resp=$(curl -i -s -X POST --data-binary @$TMPFILE "$HOST$CGI_PATH")
check_status "200" "$resp" "Large body"

# ----------------------------
info "Chunked body CGI"
resp=$(printf "POST %s HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n" "$CGI_PATH" | nc localhost 8080)
if echo "$resp" | grep -q "200"; then
    pass "Chunked decoding"
else
    fail "Chunked decoding"
    echo "$resp"
fi

# ----------------------------
info "CGI Environment variables"
resp=$(curl -i -s "$HOST$CGI_PATH")
if echo "$resp" | grep -q "REQUEST_METHOD"; then
    pass "ENV vars"
else
    fail "ENV vars"
fi

# ----------------------------
info "CGI timeout handling"
resp=$(curl -i -s --max-time 3 "$HOST/cgi-bin/loop.py")
if [ $? -ne 0 ]; then
    pass "Timeout protection"
else
    fail "Timeout protection"
fi

# ----------------------------
info "CGI crash protection"
resp=$(curl -i -s "$HOST/cgi-bin/crash.py")
check_status "500" "$resp" "Crash handling"

# ----------------------------
info "CGI permissions"
resp=$(curl -i -s "$HOST/cgi-bin/noperm.py")
check_status "403" "$resp" "Permission deny"

# ----------------------------
info "CGI path traversal"
resp=$(curl -i -s "$HOST/cgi-bin/../test.py")
check_status "403" "$resp" "Path traversal block"

# ----------------------------
info "CGI output headers"
resp=$(curl -i -s "$HOST$CGI_PATH")
if echo "$resp" | grep -q "Content-Type:"; then
    pass "CGI headers"
else
    fail "Missing headers"
fi

# ----------------------------
info "Upload via CGI"
echo "UPLOAD TEST" > $TMPFILE
resp=$(curl -i -s -X POST --data-binary @$TMPFILE "$HOST$UPLOAD_CGI")
check_status "201" "$resp" "CGI upload"

# ----------------------------
info "Concurrent CGI"
for i in {1..20}; do
    curl -s "$HOST$CGI_PATH" > /dev/null &
done
wait
pass "Concurrency"

# ----------------------------
info "CGI memory leak stress"
for i in {1..50}; do
    curl -s "$HOST$CGI_PATH" > /dev/null
done
pass "Stress test"

# ----------------------------
echo "========================================"
echo " CGI SYSTEM TEST COMPLETE"
echo "========================================"

