#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
//for dev B
struct Request {
    std::string method;   // "GET", "POST", "DELETE"
    std::string path;     // "/upload/file.txt"
    std::string version;  // "HTTP/1.1"

    std::map<std::string, std::string> headers;
    std::string body;     // fully read body (may be empty)
};

#endif
