#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

struct Response {
    int status;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string toString() const;
};

// helper factory
Response make_error_response(int code, const std::string& message = "");
Response make_200_ok(const std::string& body, const std::string& content_type = "text/html");

#endif
