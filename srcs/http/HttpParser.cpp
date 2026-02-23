#include "../../includes/router/HttpParser.hpp"

HttpParser::HttpParser() : headerCount(0), state(REQUEST_LINE),buffer(""), errorStatus(HTTP_OK) , chunkSize(0)
{
    request.method = "";
    request.path = "";
    request.version = "";
    request.headers.clear();
    request.body = "";
    isChunked = false;
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
        errorStatus = HTTP_NOT_IMPLEMENTED; //501
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

        if (sep > MAX_LINE_SIZE)
        {
            state = ERROR;
            errorStatus = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE; //431
            return true;
        }

        std::string line = buffer.substr(0, sep);
        buffer.erase(0, sep + 2);

        if (line.empty()) //end of headers
        {
            if (isChunked)
            {
                state = BODY;
                return true;
            }
            //content length exist
            if (request.headers.count("content-length"))
            {
                long long contentlength;
                std::istringstream iss(request.headers["content-length"]);
                if (!(iss >> contentlength) || contentlength < 0 || !iss.eof())
                {
                    state = ERROR;
                    errorStatus = HTTP_BAD_REQUEST; //400
                    return true;
                }
                if (contentlength > 0)
                    state = BODY;
                else
                    state = COMPLETE;
                return true;
            }
            //POST without content length 411
            // For other methods, no body is expected
            if (request.method == "POST")
            {
                state = ERROR;
                errorStatus = HTTP_LENGTH_REQUIRED; //411
                return true;
            }

            state = COMPLETE;
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

        if (key == "transfer-encoding" && value.find("chunked") != std::string::npos)
            isChunked = true;
        }
}
//Read exactly Content-Length bytes
bool HttpParser::parseBody()
{
    if (isChunked == true)
    {
        //transfer-encoding and content-length can't both exist in header
        if (request.headers.count("content-length"))
        {
            state = ERROR;
            errorStatus = HTTP_BAD_REQUEST; //400
            return true;
        }

        //keep reading until full body is complete
        while (true)
        {
            //first read the size
            if (chunkSize == 0)
            {
                //find CRLF
                size_t sep = buffer.find("\r\n");
                if (sep == std::string::npos)
                    return (false); //wait for more data
                    
                //extract same size
                std::string data = buffer.substr(0, sep);
                buffer.erase(0, sep + 2);//remove chunk size and \r\n

                std::istringstream iss(data);
                iss >> std::hex >> chunkSize; //read chunk size

                //invalid chunk size
                if (!iss.eof() || iss.fail())
                {
                    state = ERROR;
                    errorStatus = HTTP_BAD_REQUEST; //400
                    return true;
                }

                //last chunk
                if (chunkSize == 0)
                {
                    //for final CRLF after last line
                    if(buffer.size() < 2)
                        return false;

                    if(buffer.substr(0, 2) != "\r\n")
                    {
                        state = ERROR;
                        errorStatus = HTTP_BAD_REQUEST; //400
                        return true;
                    }
                    buffer.erase(0, 2);//remove final \r\n
                    state = COMPLETE;
                    return true;
                }
            }

            //read data
            else
            {
                //until full chunk and CRLF, whole line
                if (buffer.size() < chunkSize + 2)
                {
                    return false;
                }
                //check CRLF after chunk data
                if (buffer[chunkSize] != '\r' || buffer[chunkSize + 1] != '\n')
                {
                    state = ERROR;
                    errorStatus = HTTP_BAD_REQUEST; //400
                    return true;
                }

                //append exact chunk bytes to body
                request.body.append(buffer, 0, chunkSize);

                if (request.body.size() > MAX_BODY_SIZE)
                {
                    state = ERROR;
                    errorStatus = HTTP_PAYLOAD_TOO_LARGE; //413
                    return true;
                }

                //remove chunk data and CRLF from buffer
                buffer.erase(0, chunkSize + 2);
                //reset to read next chunk size
                chunkSize = 0;
            }
        }
    }

    std::map<std::string, std::string>::iterator it =
    request.headers.find("content-length");

    if (it == request.headers.end())
    {
        state = ERROR;
        errorStatus = HTTP_LENGTH_REQUIRED; //411
        return true;
    }

    long long contentlength;
    std::istringstream iss(it->second);

    if (!(iss >> contentlength) || contentlength < 0 || !iss.eof())
    {
        state = ERROR;
        errorStatus = HTTP_BAD_REQUEST;
        return true;
    }

    if (contentlength > static_cast<long long>(MAX_BODY_SIZE))
    {
        state = ERROR;
        errorStatus = HTTP_PAYLOAD_TOO_LARGE; //413
        return true;
    }

    // Wait for enough data
    if(buffer.size() < static_cast<size_t>(contentlength))
        return false;

    // Extract EXACTLY Content-Length bytes
    request.body = buffer.substr(0, contentlength);
    buffer.erase(0, contentlength);
    
    // buffer may still have extra data (next request) treated as pipeline
    // It will be processed after reset()
    
    state = COMPLETE;
    return true;
}


void HttpParser::processBuffer()
{
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