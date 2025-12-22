#include "response.hpp"
#include <sstream>
#include <string>

static std::string status_message_from_code(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

std::string Response::toString() const {
    std::stringstream ss;
    ss << "HTTP/1.1 " << status << " " << status_message << "\r\n";
    // ensure Content-Length exists
    std::map<std::string, std::string> hdrs = headers;
    if (hdrs.find("Content-Length") == hdrs.end()) {
        std::stringstream len;
        len << body.size();
        hdrs["Content-Length"] = len.str();
    }
    // default Content-Type if missing
    if (hdrs.find("Content-Type") == hdrs.end()) {
        hdrs["Content-Type"] = "text/html";
    }
    for (std::map<std::string, std::string>::const_iterator it = hdrs.begin(); it != hdrs.end(); ++it) {
        ss << it->first << ": " << it->second << "\r\n";
    }
    ss << "\r\n";
    ss << body;
    return ss.str();
}

Response make_error_response(int code, const std::string& message) {
    Response r;
    r.status = code;
    r.status_message = status_message_from_code(code);
    std::stringstream ss;
    ss << "<html><head><title>" << code << " " << r.status_message << "</title></head>\n";
    ss << "<body><h1>" << code << " " << r.status_message << "</h1>\n";
    if (!message.empty()) ss << "<p>" << message << "</p>\n";
    ss << "<hr><p>Webserv (dev C) default error page</p></body></html>\n";
    r.body = ss.str();
    r.headers["Content-Type"] = "text/html";
    return r;
}

Response make_200_ok(const std::string& body, const std::string& content_type) {
    Response r;
    r.status = 200;
    r.status_message = status_message_from_code(200);
    r.body = body;
    r.headers["Content-Type"] = content_type;
    return r;
}
