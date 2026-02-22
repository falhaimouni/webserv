# Web Server: Developer C - Handlers + CGI + Response Builder

## Complete Concept & Implementation Guide

### Developer C Responsibilities
- ✔ Static file handler
- ✔ Directory listing
- ✔ Upload handler
- ✔ CGI execution
- ✔ Response builder
- ✔ Error pages

---

## Table of Contents
1. [Static File Handler](#1-static-file-handler)
2. [Directory Listing (Autoindex)](#2-directory-listing-autoindex)
3. [Upload Handler](#3-upload-handler)
4. [CGI Execution](#4-cgi-execution)
5. [Response Builder](#5-response-builder)
6. [Error Pages](#6-error-pages)
7. [Integrated Workflow](#integrated-workflow-request--response)
8. [Security Considerations](#key-security-considerations)

---

## 1. STATIC FILE HANDLER

### Concept
The static file handler serves files directly from disk (HTML, CSS, JS, images, etc.) without processing them. It's the simplest request type but needs to handle:
- File existence checks
- Permission validation
- MIME type detection
- Efficient reading

### Implementation Flow

```cpp
// filepath: srcs/handlers/StaticHandler.cpp

StaticHandler::handle(Request& req)
├─ Check if file exists (stat())
├─ Check if readable (access())
├─ Determine MIME type
├─ Read file content
├─ Build HTTP response
└─ Return response with 200 OK
```

### Key Points

**File System Operations:**
- `stat()` - Check file existence and get size
- `open()` + `read()` - Load file into memory
- MIME type mapping (`.html` → `text/html`, `.jpg` → `image/jpeg`)
- Return 404 if file doesn't exist
- Return 403 if permission denied

**Example Response:**
```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1245
Connection: close

<!DOCTYPE html>...
```

### Implementation Details

```cpp
// Check if file exists and get metadata
struct stat file_stat;
if (stat(filepath.c_str(), &file_stat) != 0) {
    return Response(404, "Not Found");
}

// Check if file is readable
if (access(filepath.c_str(), R_OK) != 0) {
    return Response(403, "Forbidden");
}

// Determine MIME type
string mime_type = getMimeType(filepath);  // "text/html", "image/jpeg", etc.

// Open and read file
int fd = open(filepath.c_str(), O_RDONLY);
if (fd < 0) {
    return Response(500, "Internal Server Error");
}

// Read file content
vector<char> buffer(file_stat.st_size);
ssize_t bytes_read = read(fd, buffer.data(), file_stat.st_size);
close(fd);

// Build response
Response response(200, "OK");
response.addHeader("Content-Type", mime_type);
response.addHeader("Content-Length", to_string(file_stat.st_size));
response.setBody(string(buffer.begin(), buffer.end()));
return response;
```

### MIME Type Detection

```cpp
string getMimeType(const string& filepath) {
    if (filepath.ends_with(".html")) return "text/html";
    if (filepath.ends_with(".css"))  return "text/css";
    if (filepath.ends_with(".js"))   return "text/javascript";
    if (filepath.ends_with(".jpg"))  return "image/jpeg";
    if (filepath.ends_with(".png"))  return "image/png";
    if (filepath.ends_with(".pdf"))  return "application/pdf";
    if (filepath.ends_with(".txt"))  return "text/plain";
    return "application/octet-stream";  // Default fallback
}
```

---

## 2. DIRECTORY LISTING (AUTOINDEX)

### Concept
When a client requests a directory path (e.g., `/uploads/`), instead of returning an error, generate an HTML page showing all files in that directory. This is optional but useful for debugging.

### Implementation Flow

```cpp
// filepath: srcs/handlers/DirectoryHandler.cpp

DirectoryHandler::handle(Request& req)
├─ Check if path is a directory (is_dir())
├─ Check if autoindex is enabled in config
├─ Read directory contents (opendir/readdir)
├─ Generate HTML with file list
│  ├─ File names
│  ├─ File sizes
│  └─ Modification dates
└─ Return HTML response
```

### HTML Generation Example

```html
<html>
<head>
    <title>Index of /uploads/</title>
</head>
<body>
<h1>Index of /uploads/</h1>
<ul>
  <li><a href="file1.txt">file1.txt</a> (1.2 KB)</li>
  <li><a href="file2.pdf">file2.pdf</a> (450 KB)</li>
  <li><a href="subdirectory/">subdirectory/</a> (DIR)</li>
</ul>
</body>
</html>
```

### Implementation Details

```cpp
string DirectoryHandler::generateDirectoryListing(
    const string& dir_path,
    const string& request_path) {
    
    string html = "<html><head><title>Index of " + request_path + 
                  "</title></head><body>";
    html += "<h1>Index of " + request_path + "</h1>";
    html += "<ul>";
    
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return "";  // Error reading directory
    }
    
    struct dirent* entry;
    vector<string> entries;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (string(entry->d_name) == ".") continue;  // Skip current dir
        
        string full_path = dir_path + "/" + entry->d_name;
        struct stat st;
        stat(full_path.c_str(), &st);
        
        string name = entry->d_name;
        if (S_ISDIR(st.st_mode)) {
            name += "/";  // Suffix directories with /
            html += "<li><a href=\"" + name + "\">" + name + "</a>";
            html += " (DIR)</li>";
        } else {
            html += "<li><a href=\"" + name + "\">" + name + "</a>";
            html += " (" + formatFileSize(st.st_size) + ")</li>";
        }
    }
    
    closedir(dir);
    html += "</ul></body></html>";
    return html;
}
```

### Security Consideration

**Important:** Only enabled if `autoindex on` in config
```conf
server {
    listen 8081;
    
    location /uploads/ {
        autoindex on;      # Enable directory listing
    }
    
    location /private/ {
        autoindex off;     # Disable (default)
    }
}
```

- Default is `autoindex off`
- Prevents exposing sensitive files
- Can be configured per location

---

## 3. UPLOAD HANDLER

### Concept
Process POST requests with file uploads. The client sends file data in the HTTP body, and the server saves it to disk.

### HTTP Upload Flow

**Client sends:**
```
POST /uploads/ HTTP/1.1
Content-Length: 5000
Content-Type: application/octet-stream

[5000 bytes of file data]
```

**Server receives:**
```
HTTP/1.1 201 Created
Location: /uploads/filename.txt
Content-Length: 0
```

### Implementation Flow

```cpp
// filepath: srcs/handlers/UploadHandler.cpp

UploadHandler::handle(Request& req)
├─ Check Content-Length <= client_max_body_size
├─ Extract filename from request
├─ Create upload directory if needed (mkdir)
├─ Open file for writing (open())
├─ Write request body to file
├─ Verify write success (fsync)
├─ Return 201 Created with Location header
└─ Handle errors:
   ├─ 413 Payload Too Large (exceeds limit)
   ├─ 507 Insufficient Storage (disk full)
   └─ 500 Internal Server Error (permission denied)
```

### Implementation Details

**1. Size Validation:**
```cpp
// Check if upload exceeds configured limit
if (req.getContentLength() > config.client_max_body_size) {
    return Response(413, "Payload Too Large");
}
```

**2. Safe Filename Handling:**
```cpp
string UploadHandler::sanitizeFilename(const string& filename) {
    string result = filename;
    
    // Remove path traversal attempts
    size_t pos = 0;
    while ((pos = result.find("..")) != string::npos) {
        result.erase(pos, 2);
    }
    
    // Remove directory separators
    pos = 0;
    while ((pos = result.find("/")) != string::npos) {
        result.replace(pos, 1, "_");
    }
    
    return result;
}
```

**3. File Creation:**
```cpp
// Use O_EXCL to fail if file already exists (atomic operation)
int fd = open(filepath.c_str(), 
              O_CREAT | O_WRONLY | O_EXCL,  // Fail if exists
              0644);                         // rw-r--r--
if (fd < 0) {
    if (errno == EEXIST) {
        return Response(409, "Conflict");    // File already exists
    } else if (errno == EACCES) {
        return Response(403, "Forbidden");   // Permission denied
    } else {
        return Response(500, "Internal Server Error");
    }
}
```

**4. Writing Data:**
```cpp
const string& body = req.getBody();
size_t total_written = 0;

while (total_written < body.size()) {
    ssize_t written = write(fd, 
                           body.c_str() + total_written,
                           body.size() - total_written);
    
    if (written < 0) {
        close(fd);
        unlink(filepath.c_str());  // Clean up partial file
        return Response(507, "Insufficient Storage");  // Disk full
    }
    total_written += written;
}

// Ensure data is written to disk
fsync(fd);
close(fd);
```

**5. Success Response:**
```cpp
Response response(201, "Created");
response.addHeader("Location", "/uploads/" + filename);
response.addHeader("Content-Length", "0");
return response;
```

### Configuration Example

```conf
server {
    listen 8081;
    client_max_body_size 10M;  # Max upload size
    
    location /uploads/ {
        limit_except GET POST {
            deny all;
        }
    }
}
```

---

## 4. CGI EXECUTION

### Concept
CGI (Common Gateway Interface) allows the server to run external programs (Python, Bash, etc.) and return their output as the HTTP response. This enables dynamic content generation.

### CGI Request Flow

**Client requests:**
```
GET /cgi-bin/test.py?param1=value1 HTTP/1.1
```

**Server executes:**
```
1. Fork a child process
2. Set environment variables (PATH_INFO, QUERY_STRING, etc.)
3. Execute the program: python3 /path/to/test.py
4. Capture stdout as response body
5. Wait for completion with timeout
6. Return output with 200 OK
```

### Implementation Flow

```cpp
// filepath: srcs/handlers/CGIHandler.cpp

CGIHandler::handle(Request& req)
├─ Validate script exists and is executable
├─ Fork child process
│  ├─ Child process:
│  │  ├─ Set environment variables
│  │  ├─ Change working directory
│  │  ├─ Redirect stdout/stdin/stderr
│  │  ├─ execve() the script
│  │  └─ Exit
│  └─ Parent process:
│     ├─ Close unused pipes
│     ├─ Read output with timeout
│     ├─ Wait for child with waitpid()
│     └─ Collect output
├─ Parse CGI output (headers + body)
├─ Build HTTP response
└─ Handle timeout (kill process if > 30s)
```

### Environment Variables Set

```bash
REQUEST_METHOD=GET
PATH_INFO=/cgi-bin/test.py
QUERY_STRING=param1=value1
CONTENT_LENGTH=0
CONTENT_TYPE=application/x-www-form-urlencoded
SERVER_NAME=localhost
SERVER_PORT=8081
SCRIPT_NAME=/cgi-bin/test.py
HTTP_USER_AGENT=curl/8.5.0
HTTP_HOST=localhost:8081
REMOTE_ADDR=127.0.0.1
```

### CGI Script Example

```python
#!/usr/bin/env python3
# filepath: www/cgi-bin/test.py

import os
import sys

# Output HTTP headers
print("Content-Type: text/html")
print("Status: 200 OK")
print()  # Blank line separates headers from body

# Output HTML body
print("<html><body>")
print(f"<h1>Hello from CGI!</h1>")
print(f"<p>Query: {os.environ.get('QUERY_STRING', 'None')}</p>")
print(f"<p>Method: {os.environ.get('REQUEST_METHOD', 'None')}</p>")
print("</body></html>")

sys.exit(0)
```

### Implementation Details

**1. Pre-execution Checks:**
```cpp
// Check if script exists
struct stat st;
if (stat(script_path.c_str(), &st) != 0) {
    return Response(404, "Not Found");
}

// Check if file is executable
if (access(script_path.c_str(), X_OK) != 0) {
    return Response(403, "Forbidden");
}

// Check if it's a regular file
if (!S_ISREG(st.st_mode)) {
    return Response(403, "Forbidden");
}
```

**2. Fork and Execute:**
```cpp
pid_t pid = fork();

if (pid == -1) {
    return Response(500, "Internal Server Error");  // Fork failed
}

if (pid == 0) {
    // CHILD PROCESS
    
    // Close unused file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect to pipes
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);
    
    // Close original pipe file descriptors
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    
    // Change to working directory
    chdir(script_dir.c_str());
    
    // Prepare arguments
    char* argv[] = {
        (char*)interpreter.c_str(),  // "python3"
        (char*)script_path.c_str(),   // "/path/to/script.py"
        nullptr
    };
    
    // Execute script
    execve(argv[0], argv, envp);
    
    // If we reach here, execve failed
    exit(1);
    
} else {
    // PARENT PROCESS
    
    // Close unused pipe ends
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    
    // ... read output with timeout
}
```

**3. Pipe Communication:**
```cpp
int stdout_pipe[2];
int stderr_pipe[2];
int stdin_pipe[2];

pipe(stdout_pipe);   // For capturing script output
pipe(stderr_pipe);   // For capturing errors
pipe(stdin_pipe);    // For sending input if needed
```

**4. Timeout Protection:**
```cpp
#include <sys/select.h>
#include <signal.h>

time_t start = time(nullptr);
const int CGI_TIMEOUT = 30;  // 30 seconds
string output;
char buffer[4096];

while (true) {
    // Check timeout
    if (time(nullptr) - start > CGI_TIMEOUT) {
        kill(pid, SIGTERM);
        usleep(100000);  // Wait 100ms for graceful shutdown
        
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            kill(pid, SIGKILL);  // Force kill if still running
        }
        return Response(504, "Gateway Timeout");
    }
    
    // Use select() for non-blocking read with timeout
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(stdout_pipe[0], &readfds);
    
    struct timeval tv;
    tv.tv_sec = 1;      // 1 second timeout per select
    tv.tv_usec = 0;
    
    int ret = select(stdout_pipe[0] + 1, &readfds, nullptr, nullptr, &tv);
    
    if (ret > 0 && FD_ISSET(stdout_pipe[0], &readfds)) {
        ssize_t n = read(stdout_pipe[0], buffer, sizeof(buffer));
        if (n == 0) break;      // EOF - process finished
        if (n > 0) output.append(buffer, n);
    } else if (ret == 0) {
        // Timeout on select - check if process still running
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result > 0) break;  // Process finished
    }
}

// Final wait for process
int status;
waitpid(pid, &status, 0);

// Check exit status
if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    return Response(500, "Internal Server Error");
}
```

**5. Output Parsing:**
```cpp
string CGIHandler::parseCGIOutput(const string& output) {
    size_t header_end = output.find("\r\n\r\n");
    if (header_end == string::npos) {
        header_end = output.find("\n\n");
        if (header_end == string::npos) {
            return output;  // No headers, return all as body
        }
    }
    
    return output.substr(header_end + 4);  // Return body only
}
```

### Error Handling

```cpp
// Check if script produces output
if (output.empty()) {
    return Response(502, "Bad Gateway");
}

// Check for common CGI errors
if (output.find("Traceback") != string::npos ||
    output.find("Error") != string::npos) {
    // Log error but still return output
}
```

---

## 5. RESPONSE BUILDER

### Concept
The Response class is responsible for building properly formatted HTTP responses. Every handler returns a Response object that contains:
- Status code (200, 404, 500, etc.)
- Headers (Content-Type, Content-Length, etc.)
- Body (file content, error page, etc.)

### HTTP Response Structure

```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1245
Connection: close
Date: Sun, 22 Feb 2026 20:51:37 GMT
Server: webserv/1.0

<!DOCTYPE html>...
```

### Response Class Design

```cpp
// filepath: includes/shared/Response.hpp

class Response {
private:
    int status_code;
    string reason_phrase;
    map<string, string> headers;
    string body;
    
public:
    Response(int code, const string& phrase = "");
    
    void setStatus(int code, const string& phrase = "");
    void addHeader(const string& key, const string& value);
    void setBody(const string& content);
    
    string serialize() const;
    
    int getStatusCode() const;
    const string& getReasonPhrase() const;
    const map<string, string>& getHeaders() const;
    const string& getBody() const;
};
```

### Implementation

```cpp
// filepath: srcs/http/Response.cpp

Response::Response(int code, const string& phrase) 
    : status_code(code), reason_phrase(phrase) {
    if (reason_phrase.empty()) {
        reason_phrase = getDefaultReasonPhrase(code);
    }
}

void Response::addHeader(const string& key, const string& value) {
    headers[key] = value;
}

void Response::setBody(const string& content) {
    body = content;
}

string Response::serialize() const {
    string output;
    
    // Status line
    output += "HTTP/1.1 " + to_string(status_code) + " " + 
             reason_phrase + "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers) {
        output += key + ": " + value + "\r\n";
    }
    
    // Auto-add Content-Length if not present
    if (headers.find("Content-Length") == headers.end() && 
        !body.empty()) {
        output += "Content-Length: " + to_string(body.length()) + "\r\n";
    }
    
    // Add default headers if missing
    if (headers.find("Date") == headers.end()) {
        output += "Date: " + getCurrentDateTime() + "\r\n";
    }
    
    if (headers.find("Server") == headers.end()) {
        output += "Server: webserv/1.0\r\n";
    }
    
    // Blank line separator
    output += "\r\n";
    
    // Body
    output += body;
    
    return output;
}

string Response::getDefaultReasonPhrase(int code) {
    map<int, string> phrases = {
        {200, "OK"},
        {201, "Created"},
        {204, "No Content"},
        {301, "Moved Permanently"},
        {304, "Not Modified"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {409, "Conflict"},
        {413, "Payload Too Large"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {507, "Insufficient Storage"}
    };
    
    auto it = phrases.find(code);
    return (it != phrases.end()) ? it->second : "Unknown";
}
```

### Response Building Examples

**1. Success Response (200 OK):**
```cpp
Response response(200, "OK");
response.addHeader("Content-Type", "text/html");
response.addHeader("Content-Length", "1245");
response.setBody("<html>...");
return response;
```

**2. Error Response (404):**
```cpp
string error_page = "<html>...</html>";
Response response(404, "Not Found");
response.addHeader("Content-Type", "text/html");
response.addHeader("Content-Length", to_string(error_page.length()));
response.setBody(error_page);
return response;
```

**3. Created Response (201):**
```cpp
Response response(201, "Created");
response.addHeader("Location", "/uploads/filename.txt");
response.addHeader("Content-Length", "0");
return response;
```

**4. Redirect Response (301):**
```cpp
Response response(301, "Moved Permanently");
response.addHeader("Location", "/new/path");
response.addHeader("Content-Length", "0");
return response;
```

### Serialization Example Output

```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1245
Date: Sun, 22 Feb 2026 20:51:37 GMT
Server: webserv/1.0
Connection: close

<!DOCTYPE html>
<html>
  <head><title>Home</title></head>
  <body><h1>Welcome</h1></body>
</html>
```

---

## 6. ERROR PAGES

### Concept
When an error occurs (404, 500, etc.), the server should return a user-friendly HTML page instead of plain text. These can be customized per server.

### Configuration

```conf
server {
    listen 8081;
    server_name localhost;
    root /var/www/html;
    
    # Custom error pages
    error_page 404 /404.html;
    error_page 500 502 503 /5xx.html;
    error_page 403 /forbidden.html;
}
```

### Implementation Flow

```cpp
// filepath: srcs/handlers/ErrorHandler.cpp

ErrorHandler::getErrorResponse(int status_code, 
                               const ServerConfig& config)
├─ Look up custom error page in config
│  ├─ If found: read and return custom HTML
│  └─ If not found: use default HTML
├─ Inject dynamic content (status code, date)
└─ Return as response body with appropriate status code
```

### Implementation Details

**1. Check Config for Custom Page:**
```cpp
Response ErrorHandler::handle(int status_code, 
                               const ServerConfig& config) {
    // Look for custom error page in config
    string error_page_path = config.getErrorPage(status_code);
    
    if (!error_page_path.empty()) {
        string custom_html = loadErrorPage(error_page_path);
        if (!custom_html.empty()) {
            Response response(status_code);
            response.addHeader("Content-Type", "text/html");
            response.setBody(custom_html);
            return response;
        }
    }
    
    // Fall back to default error page
    string default_html = getDefaultErrorPage(status_code);
    
    Response response(status_code);
    response.addHeader("Content-Type", "text/html");
    response.setBody(default_html);
    return response;
}
```

**2. Load Custom Error Page:**
```cpp
string ErrorHandler::loadErrorPage(const string& filepath) {
    ifstream file(filepath);
    
    if (!file.good()) {
        return "";  // File doesn't exist or can't read
    }
    
    // Read entire file
    string content((istreambuf_iterator<char>(file)),
                   istreambuf_iterator<char>());
    
    return content;
}
```

**3. Generate Default Error Page:**
```cpp
string ErrorHandler::getDefaultErrorPage(int code) {
    string reason = getReasonPhrase(code);
    
    return "<!DOCTYPE html>\n"
           "<html>\n"
           "<head>\n"
           "    <title>" + to_string(code) + " " + reason + "</title>\n"
           "    <style>\n"
           "        body { font-family: Arial; text-align: center; "
           "margin-top: 50px; }\n"
           "        h1 { color: #fc8b6f; font-size: 72px; }\n"
           "        p { color: #666; font-size: 18px; }\n"
           "    </style>\n"
           "</head>\n"
           "<body>\n"
           "    <h1>" + to_string(code) + "</h1>\n"
           "    <p>" + reason + "</p>\n"
           "</body>\n"
           "</html>";
}
```

### Example Custom Error Page

```html
<!-- filepath: error/404.html -->
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>404 Not Found</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 
                         Roboto, 'Helvetica Neue', Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%);
        }
        .container {
            text-align: center;
            background: white;
            padding: 50px;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.3);
        }
        h1 {
            font-size: 72px;
            margin: 0;
            color: #fc8b6f;
        }
        p {
            font-size: 18px;
            color: #666;
            margin: 20px 0 0 0;
        }
        a {
            display: inline-block;
            margin-top: 30px;
            padding: 10px 20px;
            background-color: #fc8b6f;
            color: white;
            text-decoration: none;
            border-radius: 5px;
        }
        a:hover {
            background-color: #fb7a5e;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>404</h1>
        <p>Page Not Found</p>
        <p style="font-size: 14px; color: #999; margin-top: 30px;">
            The page you're looking for doesn't exist.
        </p>
        <a href="/">Go Home</a>
    </div>
</body>
</html>
```

### Error Status Code Handling

```cpp
// Different status codes require different error pages
map<int, string> error_descriptions = {
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {409, "Conflict"},
    {413, "Payload Too Large"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {507, "Insufficient Storage"}
};
```

---

## INTEGRATED WORKFLOW: REQUEST → RESPONSE

Here's how all components work together for a complete request cycle:

```
1. HTTP REQUEST ARRIVES
   └─ Client sends: GET /file.html HTTP/1.1
   
2. REQUEST PARSING (HttpParser)
   └─ Parse method, path, headers, body
   
3. REQUEST ROUTING (RequestRouter)
   └─ Determine which handler should process this request:
      ├─ Is it a CGI path? → CGIHandler
      ├─ Is it a directory? → DirectoryHandler
      ├─ Is it an upload? → UploadHandler
      ├─ Is it a regular file? → StaticHandler
      └─ Is it an error? → ErrorHandler
      
4. HANDLER EXECUTION
   ├─ StaticHandler:
   │  ├─ Check file exists (stat)
   │  ├─ Check permissions (access)
   │  ├─ Determine MIME type
   │  ├─ Read file into memory
   │  └─ Return Response(200, file_content)
   │
   ├─ DirectoryHandler:
   │  ├─ Check if autoindex enabled
   │  ├─ List directory contents
   │  ├─ Generate HTML listing
   │  └─ Return Response(200, html)
   │
   ├─ UploadHandler:
   │  ├─ Validate Content-Length
   │  ├─ Create file safely
   │  ├─ Write body to file
   │  └─ Return Response(201, Location header)
   │
   ├─ CGIHandler:
   │  ├─ Validate script executable
   │  ├─ Fork child process
   │  ├─ Set environment variables
   │  ├─ Execute script with timeout
   │  ├─ Collect output
   │  └─ Return Response(200, script_output)
   │
   └─ ErrorHandler:
      ├─ Look for custom error page
      ├─ Generate HTML from template
      └─ Return Response(status_code, error_html)
      
5. RESPONSE BUILDER FORMATS
   ├─ Set status code and reason phrase
   ├─ Add headers:
   │  ├─ Content-Type
   │  ├─ Content-Length
   │  ├─ Date
   │  ├─ Server
   │  └─ Any custom headers
   ├─ Set body content
   └─ Serialize to HTTP format:
      └─ "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n...\r\n\r\n[body]"
      
6. RESPONSE SENT TO CLIENT
   └─ Via non-blocking send()
      └─ "HTTP/1.1 200 OK\r\n..." + file content
```

### Complete Example: Serving a File

```
Client Request:
GET /index.html HTTP/1.1
Host: localhost:8081

↓ (RequestRouter routes to StaticHandler)

Server Process:
1. stat("/var/www/html/index.html") → exists, regular file, size=1245
2. access("/var/www/html/index.html", R_OK) → readable
3. getMimeType("index.html") → "text/html"
4. Read 1245 bytes from file
5. Build Response:
   ├─ status_code = 200
   ├─ headers["Content-Type"] = "text/html"
   ├─ headers["Content-Length"] = "1245"
   ├─ body = file content

Server Response:
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1245
Date: Sun, 22 Feb 2026 20:51:37 GMT
Server: webserv/1.0
Connection: close

<!DOCTYPE html>
<html>
...content...
</html>
```

---

## KEY SECURITY CONSIDERATIONS

### 1. Path Traversal Protection

**Problem:** Attacker uses `..` to escape root directory
```
GET /../../etc/passwd HTTP/1.1  ← Try to read system files
```

**Solution:**
```cpp
bool isPathSafe(const string& filepath, const string& root) {
    // Resolve symlinks and relative paths
    char resolved[PATH_MAX];
    if (realpath(filepath.c_str(), resolved) == nullptr) {
        return false;  // File doesn't exist
    }
    
    // Check if resolved path starts with root
    if (strncmp(resolved, root.c_str(), strlen(root.c_str())) != 0) {
        return false;  // Path escapes root
    }
    
    return true;
}
```

### 2. File Permissions

```cpp
// Always check permissions before accessing files
if (access(filepath.c_str(), R_OK) != 0) {
    return Response(403, "Forbidden");
}

// Don't serve files with wrong permissions
struct stat st;
stat(filepath.c_str(), &st);
if ((st.st_mode & 0444) == 0) {  // Not readable
    return Response(403, "Forbidden");
}
```

### 3. Body Size Limits

**Problem:** Huge uploads exhaust disk space or memory
```
POST /upload HTTP/1.1
Content-Length: 1000000000  ← 1 GB
```

**Solution:**
```cpp
if (req.getContentLength() > config.client_max_body_size) {
    return Response(413, "Payload Too Large");
}

// Config example:
// client_max_body_size 10M;  // Max 10 MB
```

### 4. CGI Timeout

**Problem:** Scripts hang indefinitely
```
GET /cgi-bin/infinite_loop.py HTTP/1.1
```

**Solution:**
```cpp
// Kill script after 30 seconds
if (time(nullptr) - start_time > 30) {
    kill(pid, SIGKILL);
    return Response(504, "Gateway Timeout");
}
```

### 5. Safe Filename Handling

**Problem:** Attacker creates files with special names
```
POST /upload HTTP/1.1

filename="../../root/.ssh/id_rsa"
```

**Solution:**
```cpp
string sanitizeFilename(const string& filename) {
    string result;
    for (char c : filename) {
        // Only allow alphanumeric, dash, underscore, dot
        if (isalnum(c) || c == '-' || c == '_' || c == '.') {
            result += c;
        }
    }
    return result;
}
```

### 6. Resource Limits

```cpp
// Limit concurrent connections
const int MAX_CONNECTIONS = 100;

// Limit request timeout
const int REQUEST_TIMEOUT = 60;  // 60 seconds

// Limit CGI output
const int MAX_CGI_OUTPUT = 10 * 1024 * 1024;  // 10 MB
```

---

## SUMMARY TABLE

| Component | Purpose | Input | Output | Status Code |
|-----------|---------|-------|--------|-------------|
| **StaticHandler** | Serve files from disk | File path | File content | 200 OK, 404, 403 |
| **DirectoryHandler** | List directory contents | Dir path | HTML listing | 200 OK, 403 |
| **UploadHandler** | Save uploaded files | POST body + filename | Created file | 201 Created, 413, 507 |
| **CGIHandler** | Execute dynamic scripts | Script path + args | Script output | 200 OK, 404, 503, 504 |
| **ErrorHandler** | Display error pages | Status code | Error HTML | Any error code |
| **ResponseBuilder** | Format HTTP responses | Status + headers + body | HTTP/1.1 response | N/A |

---

## Key Takeaways

Your implementation handles:

✅ **Static content delivery** - Efficient file serving with proper MIME types  
✅ **Dynamic content** - CGI script execution with timeout protection  
✅ **User uploads** - Safe file handling with size limits  
✅ **Directory browsing** - HTML directory listings (optional)  
✅ **Error handling** - Custom error pages per status code  
✅ **HTTP compliance** - Proper response formatting and headers  
✅ **Security** - Path traversal protection, permission checks, timeout guards  

This covers the complete server-side request processing pipeline! 🎯

---

*Last Updated: February 23, 2026*  
*For: Web Server Project - Developer C Responsibilities*
