#include "../../includes/router/HttpParser.hpp"

HttpParser::HttpParser() : headerCount(0), state(REQUEST_LINE),buffer(""), errorStatus(HTTP_OK)
{
    request.method = "";
    request.path = "";
    request.version = "";
    request.headers.clear();
    request.body = "";
}

//chunk is raw bytes from recv(), from A
//add new bytes and try to parse as much as possible
void HttpParser::feed(const std::string& chunk)
{
    buffer += chunk;
    while (state != COMPLETE && state != ERROR)
    {
        if (state == REQUEST_LINE)
        {
            if (!parseRequestLine())
                break;
        }
        else if (state == HEADERS)
        {
            if (!parseHeaders())
                break;
        }
        else if (state == BODY)
        {
            if (!parseBody())
                break;
        }
    }
}

bool HttpParser::parseRequestLine()
{
    size_t sep = buffer.find("\r\n");
    if (sep == std::string::npos)
    {
        if (buffer.size() > MAX_LINE_SIZE)
        {
            state = ERROR;
            errorStatus = HTTP_REQUEST_URI_TOO_LONG; //414
            return (true);
        }
        return (false);
    }

    if (sep > MAX_LINE_SIZE)
    {
        state = ERROR;
        errorStatus = HTTP_REQUEST_URI_TOO_LONG;
        return true;
    }

    std::string line = buffer.substr(0, sep);
    buffer.erase(0, sep + 2);

    //split by spaces
    std::istringstream iss(line);

    if (!(iss >> request.method >> request.path >> request.version))
    {
        state = ERROR;
        errorStatus = HTTP_BAD_REQUEST; //400
        return true;
    }

    std::string extra;
    if (iss >> extra)
    {
        state = ERROR;
        errorStatus = HTTP_BAD_REQUEST; //400
        return true;
    }
    if (!validateVersion())
    {
        state = ERROR;
        errorStatus = HTTP_VERSION_NOT_SUPPORTED; //505
        return true;
    }

    if (!validateMethod())
    {
        state = ERROR;
        errorStatus = HTTP_METHOD_NOT_ALLOWED; //405
        return true;
    }

    state = HEADERS;
    return true;
}


bool HttpParser::parseHeaders()
{
    while (true)
    {
        size_t sep = buffer.find("\r\n");
        if (sep == std::string::npos)
        {
            if (buffer.size() > MAX_LINE_SIZE)
            {
                state = ERROR;
                errorStatus = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE; //431
                return (true);
            }
            return (false);
        }
        std::string line = buffer.substr(0, sep);
        buffer.erase(0, sep + 2);
    
        if (line.empty()) //end of headers
        {
            // Check if ANY method has Content-Length
            if (request.headers.count("content-length"))
            {
                int contentlength;
                std::istringstream iss(request.headers["content-length"]);
                if (!(iss >> contentlength) || contentlength < 0 || !iss.eof())
                {
                    state = ERROR;
                    errorStatus = HTTP_BAD_REQUEST;
                    return true;
                }
                state = BODY;
            }
            else
            {
                // No body
                if (request.method == "POST")
                {
                    state = ERROR; // POST without Content-Length is invalid
                    errorStatus = HTTP_LENGTH_REQUIRED; //411
                    return true;
                }
                state = COMPLETE;
            }
            return true;
        }
        //normal header line
        size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            state = ERROR;
            errorStatus = HTTP_BAD_REQUEST;
            return true;
        }

        headerCount++;
        if (headerCount > MAX_HEADERS)
        {
            state = ERROR;
            errorStatus = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE; //431
            return true;
        }

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));

        if (key.empty())
        {
            state = ERROR;
            errorStatus = HTTP_BAD_REQUEST;
            return true;
        }

        for (size_t i = 0; i < key.size(); ++i)
            key[i] = std::tolower(static_cast<unsigned char>(key[i]));
        
        if (request.headers.find(key) == request.headers.end()) //in case of duplicate headers keep first
            request.headers[key] = value;
    }
}

//Read exactly Content-Length bytes
bool HttpParser::parseBody()
{
    std::map<std::string, std::string>::iterator it =
        request.headers.find("content-length");

    if (it == request.headers.end())
    {
        state = ERROR;
        errorStatus = HTTP_LENGTH_REQUIRED; //411
        return true;
    }

    int contentlength;
    std::istringstream iss(it->second);

    if (!(iss >> contentlength) || contentlength < 0 || !iss.eof())
    {
        state = ERROR;
        errorStatus = HTTP_BAD_REQUEST;
        return true;
    }

    if (contentlength > static_cast<int>(MAX_BODY_SIZE))
    {
        state = ERROR;
        errorStatus = HTTP_PAYLOAD_TOO_LARGE; //413
        return true;
    }

    // Wait for enough data
    if (buffer.size() < static_cast<size_t>(contentlength))
        return false;

    // Extract EXACTLY Content-Length bytes
    request.body = buffer.substr(0, contentlength);
    buffer.erase(0, contentlength);
    
    // buffer may still have data (next request) - this is OK!
    // It will be processed after reset()

    state = COMPLETE;
    return true;
}
