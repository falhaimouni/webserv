#include "../../includes/router/HttpParser.hpp"

HttpParser::HttpParser() : state(REQUEST_LINE), buffer("")
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
        return (false);
    std::string line = buffer.substr(0, sep);
    buffer.erase(0, sep + 2);

    //split by spaces
    std::istringstream iss(line);

    if (!(iss >> request.method >> request.path >> request.version))
    {
        state = ERROR;
        return true;
    }

    std::string extra;
    if (iss >> extra)
    {
        state = ERROR;
        return true;
    }
    if (!validateMethod() || !validateVersion())
    {
        state = ERROR;
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
            return (false);
        std::string line = buffer.substr(0, sep);
        buffer.erase(0, sep + 2);
    
        if (line.empty()) //end of headers
        {
            if (request.method == "POST")
            {
                if (!request.headers.count("Content-Length"))
                {
                    state = ERROR;
                    return true;
                }

                int contentlength;
                std::istringstream iss(request.headers["Content-Length"]);
                if (!(iss >> contentlength) || contentlength < 0 || !iss.eof())
                {
                    state = ERROR;
                    return true;
                }
                state = BODY;
            }
            else //GET or DELETE
            {
                state = COMPLETE;
            }
            return true;
        }
        //normal header line
        size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            state = ERROR;
            return true;
        }
        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));

        if (key.empty())
        {
            state = ERROR;
            return true;
        }

        request.headers[key] = value;
    }
}

//Read exactly Content-Length bytes
bool HttpParser::parseBody()
{
    std::map<std::string, std::string>::iterator it =
        request.headers.find("Content-Length");

    if (it == request.headers.end())
    {
        state = ERROR;
        return true;
    }

    int contentlength;
    std::istringstream iss(it->second);

    // parse integer AND ensure nothing extra exists
    if (!(iss >> contentlength) || contentlength < 0 || !iss.eof())
    {
        state = ERROR;
        return true;
    }

    // not enough body yet → wait
    if (buffer.size() < static_cast<size_t>(contentlength))
        return false;

    // STRICT CHECK: body must be EXACTLY Content-Length
    if (buffer.size() != static_cast<size_t>(contentlength))
    {
        state = ERROR;
        return true;
    }

    request.body = buffer.substr(0, contentlength);
    buffer.erase(0, contentlength);

    if (!buffer.empty())
    {
        state = ERROR;
        return true;
    }

    state = COMPLETE;
    return true;
}
