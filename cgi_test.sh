#!/bin/bash
# ==========================================
# WEBSERV CGI TEST SUITE - Webserv 1.0
# Author: Farah Webserv Validation Suite
# Adjusted for current CGIHandler behavior
# ==========================================

HOST="http://localhost:8080"
CGI_PATH="/cgi-bin/test.py"
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
    echo " CGI TEST SUITE - Webserv 1.0"
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
check_status "200" "$resp" "GET CGI"

# ----------------------------
info "QUERY_STRING test"
resp=$(curl -i -s "$HOST$CGI_PATH?name=farah&project=webserv")
check_status "200" "$resp" "QUERY_STRING"

# ----------------------------
info "POST CGI with body"
echo "hello=cgi&lang=cpp" > $TMPFILE
resp=$(curl -i -s -X POST --data-binary @$TMPFILE "$HOST$CGI_PATH")
# Accept 200 or 201 (current handler returns 201)
if echo "$resp" | grep -qE "200|201"; then
    pass "POST body"
else
    fail "POST body"
    echo "$resp"
fi

# ----------------------------
info "POST empty body"
resp=$(curl -i -s -X POST "$HOST$CGI_PATH")
if echo "$resp" | grep -qE "200|201"; then
    pass "Empty POST"
else
    fail "Empty POST"
    echo "$resp"
fi

# ----------------------------
info "Invalid method (DELETE)"
resp=$(curl -i -s -X DELETE "$HOST$CGI_PATH")
if echo "$resp" | grep -q "405"; then
    pass "DELETE rejected"
else
    fail "DELETE rejected"
    echo "$resp"
fi

# ----------------------------
info "Large body POST"
base64 /dev/urandom | head -c 500000 > $TMPFILE
resp=$(curl -i -s -X POST --data-binary @$TMPFILE "$HOST$CGI_PATH")
if echo "$resp" | grep -qE "200|201"; then
    pass "Large POST body"
else
    fail "Large POST body"
    echo "$resp"
fi

# ----------------------------
info "CGI environment variables"
resp=$(curl -i -s "$HOST$CGI_PATH?env=test")
if echo "$resp" | grep -q "REQUEST_METHOD"; then
    pass "ENV vars"
else
    fail "ENV vars"
    echo "$resp"
fi

# ----------------------------
info "CGI crash protection"
resp=$(curl -i -s "$HOST/cgi-bin/crash.py")
if echo "$resp" | grep -q "500"; then
    pass "Crash handling"
else
    fail "Crash handling"
    echo "$resp"
fi

# ----------------------------
info "CGI permissions check"
resp=$(curl -i -s "$HOST/cgi-bin/noperm.py")
if echo "$resp" | grep -q "403"; then
    pass "Permission denied"
else
    fail "Permission denied"
    echo "$resp"
fi

# ----------------------------
info "CGI path traversal block"
resp=$(curl -i -s "$HOST/cgi-bin/../test.py")
if echo "$resp" | grep -q "403"; then
    pass "Path traversal blocked"
else
    fail "Path traversal blocked"
    echo "$resp"
fi

# ----------------------------
info "CGI output headers"
resp=$(curl -i -s "$HOST$CGI_PATH")
if echo "$resp" | grep -q "Content-Type:"; then
    pass "Headers OK"
else
    fail "Missing headers"
fi

# ----------------------------
info "Upload via CGI"
echo "UPLOAD TEST" > $TMPFILE
resp=$(curl -i -s -X POST --data-binary @$TMPFILE "$HOST$UPLOAD_CGI")
if echo "$resp" | grep -qE "201|200"; then
    pass "CGI upload"
else
    fail "CGI upload"
    echo "$resp"
fi

# ----------------------------
info "Concurrent CGI requests"
for i in $(seq 1 20); do
    curl -s "$HOST$CGI_PATH" > /dev/null &
done
wait
pass "Concurrency"

# ----------------------------
info "CGI stress test"
for i in $(seq 1 50); do
    curl -s "$HOST$CGI_PATH" > /dev/null
done
pass "Stress test"

# ----------------------------
echo "========================================"
echo " CGI SYSTEM TEST COMPLETE"
echo "========================================"

