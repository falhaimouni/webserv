#!/bin/bash

# --------------------------
# Webserv Automated Test Script
# --------------------------
# Usage: ./test_webserv.sh [PORT]
# Default PORT=8080
# --------------------------

PORT=${1:-8080}
SERVER="http://127.0.0.1:$PORT"

echo "=== Webserv Automated Tests ==="
echo "Testing server on $SERVER"
echo ""

# Helper function
pass_fail() {
    if [ $1 -eq 0 ]; then
        echo "✅ $2"
    else
        echo "❌ $2"
    fi
}

# --------------------------
# 1. Test static file (GET)
# --------------------------
echo "1. Testing static file (test.html)"
curl -s -o tmp_output.html $SERVER/test.html
diff tmp_output.html www/test.html > /dev/null
pass_fail $? "Static file GET"

# --------------------------
# 2. Test directory listing
# --------------------------
echo "2. Testing directory listing (/testdir)"
curl -s -o tmp_dir.html $SERVER/testdir/
grep -q "<a href=" tmp_dir.html
pass_fail $? "Directory listing"

# --------------------------
# 3. Test 404 error
# --------------------------
echo "3. Testing 404 error"
curl -s -o tmp_404.html -w "%{http_code}" $SERVER/nonexistent.html | grep -q "404"
pass_fail $? "404 page"

# --------------------------
# 4. Test CGI - Python
# --------------------------
echo "4. Testing CGI (test.py)"
curl -s -o tmp_cgi.html $SERVER/test.py
grep -q "Hello CGI" tmp_cgi.html
pass_fail $? "CGI execution"

# --------------------------
# 5. Test file upload
# --------------------------
echo "5. Testing file upload (/uploads)"
echo "This is a test upload" > tmp_upload.txt
curl -s -F "file=@tmp_upload.txt" $SERVER/uploads/ -o /dev/null
grep -q "tmp_upload.txt" www/uploads/tmp_upload.txt 2>/dev/null
pass_fail $? "File upload"

# --------------------------
# 6. Test DELETE method
# --------------------------
echo "6. Testing DELETE method"
touch www/uploads/delete_test.txt
curl -s -X DELETE $SERVER/uploads/delete_test.txt -o /dev/null
[ ! -f www/uploads/delete_test.txt ]
pass_fail $? "DELETE request"

# --------------------------
# 7. Test POST form to CGI
# --------------------------
echo "7. Testing POST to CGI"
curl -s -d "name=Farah&project=Webserv" $SERVER/test.py -o tmp_post_cgi.html
grep -q "Hello CGI" tmp_post_cgi.html
pass_fail $? "POST CGI request"

# --------------------------
# 8. Test autoindex off on root
# --------------------------
echo "8. Testing autoindex off on /"
curl -s $SERVER/ | grep -q "Index of"
if [ $? -eq 0 ]; then
    echo "❌ Autoindex off on root"
else
    echo "✅ Autoindex off on root"
fi

# --------------------------
# Cleanup
# --------------------------
rm -f tmp_*.html tmp_upload.txt

echo ""
echo "=== Testing Completed ==="