# Webserv Testing Guide

## Quick Start

1. **Build the server:**
   ```bash
   make
   ```

2. **Run the server:**
   ```bash
   ./webserver
   ```
   Server will run on port 8080.

3. **In another terminal, test with curl or browser**

---

## Test Cases

### 1. Static File Serving

**Test basic file serving:**
```bash
curl -v http://localhost:8080/index.html
```

**Test different file types:**
```bash
# Create test files
echo "<h1>Test HTML</h1>" > www/test.html
echo "body { color: red; }" > www/style.css
echo "console.log('test');" > www/script.js

# Test them
curl http://localhost:8080/test.html
curl http://localhost:8080/style.css
curl http://localhost:8080/script.js
```

**Test MIME types:**
```bash
curl -I http://localhost:8080/test.html  # Should show text/html
curl -I http://localhost:8080/style.css  # Should show text/css
curl -I http://localhost:8080/script.js  # Should show application/javascript
```

### 2. Directory Listing

**Test directory listing:**
```bash
# Create a test directory with files
mkdir -p www/testdir
echo "file1" > www/testdir/file1.txt
echo "file2" > www/testdir/file2.txt

# Test directory listing
curl http://localhost:8080/testdir/
```

**Expected:** HTML page showing directory contents with file sizes and dates.

### 3. Index Files

**Test index file support:**
```bash
# Create directory with index.html
mkdir -p www/mydir
echo "<h1>Index Page</h1>" > www/mydir/index.html

# Request directory (should serve index.html)
curl http://localhost:8080/mydir/
curl http://localhost:8080/mydir
```

### 4. Upload Functionality

**Test simple POST upload:**
```bash
# Upload a file
echo "Hello, this is uploaded content!" > test_upload.txt
curl -X POST -d @test_upload.txt http://localhost:8080/uploads/test.txt

# Verify it was uploaded
cat www/uploads/test.txt  # or wherever upload_store is configured
```

**Test multipart/form-data upload:**
```bash
curl -X POST \
  -F "file=@test_upload.txt" \
  -F "name=testfile" \
  http://localhost:8080/uploads/
```

**Test file size limit:**
```bash
# Create a large file (if max body size is 1MB)
dd if=/dev/zero of=large_file.bin bs=1M count=2
curl -X POST -d @large_file.bin http://localhost:8080/uploads/large.bin
# Should return 413 Request Entity Too Large
```

### 5. CGI Execution

**Test Python CGI:**
```bash
# Create a Python CGI script
cat > www/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html\n")
print("<h1>Hello from Python CGI!</h1>")
print("<p>QUERY_STRING:", __import__('os').environ.get('QUERY_STRING', ''), "</p>")
EOF

chmod +x www/cgi-bin/test.py

# Test it
curl http://localhost:8080/cgi-bin/test.py
curl "http://localhost:8080/cgi-bin/test.py?name=test&value=123"
```

**Test PHP CGI (if PHP is installed):**
```bash
# Create a PHP script
cat > www/cgi-bin/test.php << 'EOF'
<?php
header("Content-Type: text/html");
echo "<h1>Hello from PHP CGI!</h1>";
echo "<p>REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<p>QUERY_STRING: " . $_SERVER['QUERY_STRING'] . "</p>";
?>
EOF

chmod +x www/cgi-bin/test.php

# Test it
curl http://localhost:8080/cgi-bin/test.php
```

**Test Bash CGI:**
```bash
# Create a bash script
cat > www/cgi-bin/test.sh << 'EOF'
#!/bin/bash
echo "Content-Type: text/html"
echo ""
echo "<h1>Hello from Bash CGI!</h1>"
echo "<p>Environment variables:</p>"
echo "<ul>"
env | grep -E '^(REQUEST_METHOD|QUERY_STRING|PATH_INFO)=' | sed 's/^/<li>/;s/$/<\/li>/'
echo "</ul>"
EOF

chmod +x www/cgi-bin/test.sh

# Test it
curl http://localhost:8080/cgi-bin/test.sh
```

**Test CGI with POST data:**
```bash
echo "test data" | curl -X POST -d @- http://localhost:8080/cgi-bin/test.py
```

### 6. Error Handling

**Test 404 Not Found:**
```bash
curl -v http://localhost:8080/nonexistent.html
# Should return 404 with styled error page
```

**Test 403 Forbidden (path traversal):**
```bash
curl -v http://localhost:8080/../../etc/passwd
# Should return 403 Forbidden
```

**Test 405 Method Not Allowed:**
```bash
curl -X DELETE http://localhost:8080/index.html
# Should return 405 if DELETE is not allowed
```

**Test 413 Request Entity Too Large:**
```bash
# Create large file and try to upload
dd if=/dev/zero of=large.bin bs=1M count=2
curl -X POST -d @large.bin http://localhost:8080/uploads/large.bin
```

**Test custom error pages:**
```bash
# Create custom error page
mkdir -p www/error_pages
echo "<h1>Custom 404 Page</h1><p>This is a custom error page!</p>" > www/error_pages/404.html

# Test it
curl http://localhost:8080/nonexistent.html
# Should show custom error page
```

### 7. Security Tests

**Test path traversal protection:**
```bash
curl http://localhost:8080/../etc/passwd
curl http://localhost:8080/../../etc/passwd
curl http://localhost:8080/....//....//etc/passwd
# All should return 403 Forbidden
```

**Test filename sanitization in uploads:**
```bash
# Try to upload with dangerous filename
curl -X POST -d "test" http://localhost:8080/uploads/../../../etc/passwd
# Should sanitize filename
```

### 8. HTTP Headers

**Test response headers:**
```bash
curl -I http://localhost:8080/index.html
# Should show:
# - HTTP/1.0 200 OK
# - Date: ...
# - Server: webserv/1.0
# - Content-Type: text/html
# - Content-Length: ...
# - Connection: close
```

**Test HTTP version:**
```bash
curl -v http://localhost:8080/index.html 2>&1 | grep "HTTP/"
# Should show HTTP/1.0
```

### 9. Request Parsing

**Test with different HTTP methods:**
```bash
curl -X GET http://localhost:8080/index.html
curl -X POST -d "data" http://localhost:8080/uploads/test.txt
curl -X PUT -d "data" http://localhost:8080/uploads/test.txt
```

**Test with headers:**
```bash
curl -H "User-Agent: TestAgent" \
     -H "Accept: text/html" \
     http://localhost:8080/index.html
```

### 10. Integration Tests

**Test full request/response cycle:**
```bash
# Create a test HTML page with links
cat > www/test.html << 'EOF'
<html>
<body>
    <h1>Test Page</h1>
    <a href="/testdir/">Directory Listing</a>
    <a href="/cgi-bin/test.py">CGI Test</a>
    <a href="/nonexistent">404 Test</a>
</body>
</html>
EOF

# Open in browser: http://localhost:8080/test.html
```

---

## Automated Testing Script

Create a test script to run all tests:

```bash
#!/bin/bash
# test_webserv.sh

SERVER_URL="http://localhost:8080"
PASSED=0
FAILED=0

test_case() {
    local name="$1"
    local cmd="$2"
    local expected="$3"
    
    echo -n "Testing $name... "
    result=$(eval "$cmd" 2>&1)
    
    if echo "$result" | grep -q "$expected"; then
        echo "✓ PASSED"
        ((PASSED++))
    else
        echo "✗ FAILED"
        echo "  Expected: $expected"
        echo "  Got: $result"
        ((FAILED++))
    fi
}

echo "Starting webserv tests..."
echo "=========================="

# Test 1: Static file serving
test_case "Static file serving" \
    "curl -s -o /dev/null -w '%{http_code}' $SERVER_URL/index.html" \
    "200"

# Test 2: 404 error
test_case "404 Not Found" \
    "curl -s -o /dev/null -w '%{http_code}' $SERVER_URL/nonexistent.html" \
    "404"

# Test 3: Directory listing
test_case "Directory listing" \
    "curl -s $SERVER_URL/testdir/ | grep -q 'Index of'" \
    ""

# Test 4: HTTP/1.0
test_case "HTTP/1.0 version" \
    "curl -s -I $SERVER_URL/index.html | grep 'HTTP/1.0'" \
    "HTTP/1.0"

# Test 5: Response headers
test_case "Date header" \
    "curl -s -I $SERVER_URL/index.html | grep -q 'Date:'" \
    ""

test_case "Server header" \
    "curl -s -I $SERVER_URL/index.html | grep -q 'Server:'" \
    ""

echo "=========================="
echo "Tests passed: $PASSED"
echo "Tests failed: $FAILED"
```

---

## Browser Testing

1. **Start the server:**
   ```bash
   ./webserver
   ```

2. **Open browser and navigate to:**
   - `http://localhost:8080/` - Root directory
   - `http://localhost:8080/index.html` - Static file
   - `http://localhost:8080/testdir/` - Directory listing
   - `http://localhost:8080/cgi-bin/test.py` - CGI script

3. **Check browser developer tools:**
   - Network tab: Verify HTTP/1.0, headers, status codes
   - Console: Check for any errors

---

## Performance Testing

**Test with multiple requests:**
```bash
# Test 100 requests
for i in {1..100}; do
    curl -s http://localhost:8080/index.html > /dev/null
done
```

**Test concurrent requests:**
```bash
# 10 concurrent requests
for i in {1..10}; do
    curl -s http://localhost:8080/index.html > /dev/null &
done
wait
```

---

## Debugging Tips

1. **Check server logs** - The server should print connection info
2. **Use curl -v** - Verbose output shows full request/response
3. **Check file permissions** - Ensure CGI scripts are executable
4. **Verify paths** - Make sure www/ directory structure matches expectations
5. **Test with telnet** - For raw HTTP testing:
   ```bash
   telnet localhost 8080
   GET /index.html HTTP/1.0
   Host: localhost:8080
   
   ```

---

## Common Issues

1. **Port already in use:** Change port in main.cpp or kill existing process
2. **CGI scripts not executing:** Check file permissions (`chmod +x`)
3. **404 on all requests:** Verify www/ directory exists and has correct structure
4. **Upload fails:** Check upload_store directory exists and is writable

