#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

struct Response {
    int status;  // 200, 404, 500...
    std::string reason; // "OK", "Not Found"

    std::map<std::string, std::string> headers;
    std::string body;
};

#endif

