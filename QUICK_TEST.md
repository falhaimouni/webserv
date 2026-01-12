# Quick Testing Guide

## 1. Start the Server

```bash
make
./webserver
```

Server runs on **port 8080**.

## 2. Basic Tests (in another terminal)

### Test Static Files
```bash
# Create a test file
echo "<h1>Hello World</h1>" > www/index.html

# Test it
curl http://localhost:8080/index.html
```

### Test Directory Listing
```bash
# Create test directory
mkdir -p www/testdir
echo "file1" > www/testdir/file1.txt
echo "file2" > www/testdir/file2.txt

# Test directory listing
curl http://localhost:8080/testdir/
```

### Test CGI (Python)
```bash
# Create Python CGI script
cat > www/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html\n")
print("<h1>CGI Works!</h1>")
EOF

chmod +x www/cgi-bin/test.py

# Test it
curl http://localhost:8080/cgi-bin/test.py
```

### Test Upload
```bash
# Upload a file
echo "Hello Upload!" | curl -X POST -d @- http://localhost:8080/uploads/test.txt

# Check if uploaded
cat www/uploads/test.txt
```

### Test Errors
```bash
# 404 error
curl -v http://localhost:8080/nonexistent.html

# Path traversal protection
curl -v http://localhost:8080/../etc/passwd
```

### Test Headers
```bash
# Check HTTP/1.0 and headers
curl -I http://localhost:8080/index.html
```

## 3. Run Automated Tests

```bash
# Make sure server is running, then:
./test_webserv.sh
```

## 4. Browser Testing

Open browser and go to:
- `http://localhost:8080/`
- `http://localhost:8080/testdir/`
- `http://localhost:8080/cgi-bin/test.py`

## 5. Common Issues

- **Port in use:** Kill existing process or change port
- **CGI not working:** Check file permissions (`chmod +x`)
- **404 on everything:** Check `www/` directory exists

