#include "../../includes/router/HttpParser.hpp"

bool HttpParser::validateMethod()
{
    return (request.method == "GET" || request.method == "POST" || request.method == "DELETE");
}

bool HttpParser::validateVersion()
{
    return request.version == "HTTP/1.0" || request.version == "HTTP/1.1";
}

std::string HttpParser::trim(const std::string& str)
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
    // buffer.clear();
    errorStatus = HTTP_OK;
    request.method.clear();
    request.path.clear();
    request.version.clear();
    request.headers.clear();
    request.body.clear();
    headerCount = 0;
    isChunked = false;
    chunkSize = 0;
    // buffer may contain next request
    processBuffer();
}

int HttpParser::getErrorStatus() const
{
    return errorStatus;
}