#include "HttpParser.hpp"

bool HttpParser::validateMethod()
{
    return (request.method == "GET" || request.method == "POST" || request.method == "DELETE");
}

bool HttpParser::validateVersion()
{
    if (request.version != "HTTP/1.0")
        return false;
    return true;
}

std::string trim(const std::string& str)
{
  size_t start = str.find_first_not_of(" \t");
  if (start == std::string::npos)
    return "";
  size_t end = str.find_last_not_of(" \t");
  return str.substr(start, end - start + 1);
}

bool HttpParser::isComplete() const
{
    return (state == COMPLETE);
}

bool HttpParser::hasError() const
{
    return (state == ERROR);
}

//only after the request is fully parsed, when isComplete == true
const Request& HttpParser::getRequest() const
{
    return (request);
}

void HttpParser::reset()
{
    state = REQUEST_LINE;
    buffer.clear();
    request.method.clear();
    request.path.clear();
    request.version.clear();
    request.headers.clear();
    request.body.clear();
}