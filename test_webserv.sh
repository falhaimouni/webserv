#!/bin/bash
# Automated test script for webserv

SERVER_URL="http://localhost:8080"
PASSED=0
FAILED=0
TOTAL=0

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

test_case() {
    local name="$1"
    local cmd="$2"
    local expected="$3"
    
    ((TOTAL++))
    echo -n "Test $TOTAL: $name... "
    
    result=$(eval "$cmd" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ] && echo "$result" | grep -q "$expected"; then
        echo -e "${GREEN}✓ PASSED${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"
        echo -e "  ${YELLOW}Expected:${NC} $expected"
        echo -e "  ${YELLOW}Got:${NC} $result"
        ((FAILED++))
        return 1
    fi
}

test_http_code() {
    local name="$1"
    local url="$2"
    local expected_code="$3"
    
    ((TOTAL++))
    echo -n "Test $TOTAL: $name... "
    
    code=$(curl -s -o /dev/null -w '%{http_code}' "$url" 2>&1)
    
    if [ "$code" = "$expected_code" ]; then
        echo -e "${GREEN}✓ PASSED${NC} (HTTP $code)"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC} (Expected HTTP $expected_code, got HTTP $code)"
        ((FAILED++))
        return 1
    fi
}

echo "=========================================="
echo "  Webserv Automated Test Suite"
echo "=========================================="
echo ""

# Check if server is running
echo "Checking if server is running..."
if ! curl -s -o /dev/null -w '%{http_code}' "$SERVER_URL" > /dev/null 2>&1; then
    echo -e "${RED}ERROR: Server is not running on $SERVER_URL${NC}"
    echo "Please start the server first: ./webserver"
    exit 1
fi
echo -e "${GREEN}Server is running${NC}"
echo ""

# Setup test files
echo "Setting up test files..."
mkdir -p www/testdir
mkdir -p www/cgi-bin
mkdir -p www/uploads
echo "test file 1" > www/testdir/file1.txt
echo "test file 2" > www/testdir/file2.txt
echo "<h1>Test HTML</h1>" > www/test.html
echo ""

# Test 1: Static file serving (200 OK)
test_http_code "Static file serving" "$SERVER_URL/test.html" "200"

# Test 2: 404 Not Found
test_http_code "404 Not Found" "$SERVER_URL/nonexistent.html" "404"

# Test 3: Directory listing
test_case "Directory listing" \
    "curl -s $SERVER_URL/testdir/ | grep -q 'Index of'" \
    ""

# Test 4: HTTP/1.0 version
test_case "HTTP/1.0 version" \
    "curl -s -I $SERVER_URL/test.html | head -1 | grep -q 'HTTP/1.0'" \
    ""

# Test 5: Response headers
test_case "Date header present" \
    "curl -s -I $SERVER_URL/test.html | grep -q 'Date:'" \
    ""

test_case "Server header present" \
    "curl -s -I $SERVER_URL/test.html | grep -q 'Server:'" \
    ""

test_case "Content-Type header present" \
    "curl -s -I $SERVER_URL/test.html | grep -q 'Content-Type:'" \
    ""

test_case "Content-Length header present" \
    "curl -s -I $SERVER_URL/test.html | grep -q 'Content-Length:'" \
    ""

# Test 6: MIME type detection
test_case "HTML MIME type" \
    "curl -s -I $SERVER_URL/test.html | grep 'Content-Type:' | grep -q 'text/html'" \
    ""

# Test 7: Path traversal protection
test_http_code "Path traversal protection" "$SERVER_URL/../etc/passwd" "403"

# Test 8: Index file (if index.html exists in root)
if [ -f "www/index.html" ]; then
    test_http_code "Index file serving" "$SERVER_URL/" "200"
fi

# Test 9: Error page styling
test_case "Error page HTML structure" \
    "curl -s $SERVER_URL/nonexistent.html | grep -q '<!DOCTYPE html>'" \
    ""

# Test 10: Directory parent link
test_case "Directory parent link" \
    "curl -s $SERVER_URL/testdir/ | grep -q '\.\.'" \
    ""

# Test 11: File metadata in directory listing
test_case "File size in directory listing" \
    "curl -s $SERVER_URL/testdir/ | grep -q 'KB\|B\|MB'" \
    ""

# Test 12: POST upload (if upload_store is configured)
test_case "POST upload" \
    "echo 'test upload' | curl -s -X POST -d @- $SERVER_URL/uploads/test_upload.txt && [ -f www/uploads/test_upload.txt ]" \
    ""

# Test 13: CGI script (if Python is available and script exists)
if command -v python3 &> /dev/null; then
    cat > www/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html\n")
print("<h1>CGI Test</h1>")
EOF
    chmod +x www/cgi-bin/test.py
    
    test_case "CGI execution" \
        "curl -s $SERVER_URL/cgi-bin/test.py | grep -q 'CGI Test'" \
        ""
fi

echo ""
echo "=========================================="
echo "  Test Results"
echo "=========================================="
echo -e "Total tests:  $TOTAL"
echo -e "${GREEN}Passed:       $PASSED${NC}"
echo -e "${RED}Failed:       $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Please review the output above.${NC}"
    exit 1
fi

