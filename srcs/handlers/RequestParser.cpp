#include "../../includes/handlers/RequestParser.hpp"
#include <sstream>

Request RequestParser::parse(const std::string &raw)
{
    Request req;
    std::string line;
    std::string body;
    std::istringstream iss(raw);

    if (std::getline(iss, line))
    {
        std::istringstream lineStream(line);
        lineStream >> req.method >> req.path >> req.version;
    }
    while (std::getline(iss, line) && line != "\r")
    {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);
            if (!value.empty() && value[value.size() - 1] == '\r')
                value.resize(value.size() - 1);
            req.headers[key] = value;
        }
    }
    while (std::getline(iss, line))
        body += line + "\n";
    if (!body.empty() && body[body.size() - 1] == '\n')
        body.resize(body.size() - 1);
    req.body = body;
    return req;
}