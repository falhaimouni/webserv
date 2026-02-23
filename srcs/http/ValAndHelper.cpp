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

//return content length header as number
long long HttpParser::getContentLength() const
{
    std::map<std::string, std::string>::const_iterator it = request.headers.find("content-length");
    //if header doesn't exist, no body length
    if (it == request.headers.end())
        return 0;
    long long length = 0;
    std::istringstream iss(it->second);
    iss >> length;
    return length;
}

size_t HttpParser::getBodySize() const
{
    return request.body.size();
}

//force the parser into error state, if client disconnect befor reading the size
void HttpParser::forceError(int status)
{
    state = ERROR;
    errorStatus = status;
}