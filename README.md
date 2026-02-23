*This project has been created as part of the 42 curriculum by jalqam, sshawish, falhaimo.*

# Description
Webserv is a C++ 98 HTTP server that helps you understand how web servers and the HTTP protocol work. It allows You to serve static websites, handle multiple clients, process HTTP requests, and execute CGI scripts.  

The server supports at least GET, POST, and DELETE methods, multiple ports, routes, and fully configurable behavior through a configuration file.  

**you run Webserv, it listens for client requests (like from a browser), reads them, and responds with the requested file, executes a CGI script, or returns an error page.**  

# Instructions

Compilation:
Use the provided Makefile to compile the project:
```bash
make
```
This will generate the executable named webserv.

Run the server with a configuration file:
```bash
./webserv path/to/configuration_file
```
Open a browser and visit:
```bash
http://localhost:8080/
```
Simple explanation of what will happen:

Webserv reads the configuration file and sets up listening sockets.

It waits for client connections using poll() (or select(), kqueue(), epoll()) in a non-blocking way.

When a client sends a request, the server parses it and checks the route, method, and file.

Webserv responds with the file content, executes a CGI script, or returns an error page if needed.

The server continues listening and handling multiple clients simultaneously without blocking.

# Resources

- [HTTP Overview - W3Schools](https://www.w3schools.com/whatis/whatis_http.asp)
- [HTTP Requests - TutorialsPoint](https://www.tutorialspoint.com/http/http_requests.htm)
- [HTTP Responses - TutorialsPoint](https://www.tutorialspoint.com/http/http_responses.htm)
- [HTTP Headers - GeeksforGeeks](https://www.geeksforgeeks.org/web-tech/http-headers/)
- [Content-Length & Headers - GeeksforGeeks](https://www.geeksforgeeks.org/computer-networks/http-headers-content-length/)
- [HTTP Chunked Encoding - IORiver](https://www.ioriver.io/terms/http-chunked-encoding)
- [Sockets - GeeksforGeeks](https://www.geeksforgeeks.org/computer-networks/socket-in-computer-network/)
- [HTTP Notes - HackMD](https://hackmd.io/@laian/SJZHcOsmT)

# AI Usage

AI tools were used by the team to support planning and organizing the project, such as:  

- Designing the overall server structure and class organization

- Assisting with project planning, debugging ideas, and workflow strategies

- Drafting documentation and notes for implementation  

All AI-generated content was **reviewed, discussed, and manually adapted** by the team to ensure full understanding and compliance with C++ 98 and project requirements.

# Features

- GET, POST, DELETE HTTP methods

- Serve static files and directories (with optional listing)

- File uploads from clients

- Execute CGI scripts (PHP, Python, etc.)

- Non-blocking architecture using poll() or equivalents

- Multiple ports and configurable routes

- Default error pages with optional custom pages

# Bonus (Optional)

- Cookie and session management examples

- Multiple CGI types beyond basic scripts