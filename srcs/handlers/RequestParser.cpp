#include "../../includes/handlers/RequestParser.hpp"
#include <sstream>

Request RequestParser::parse(const std::string &raw)
{
    Request req;

    size_t pos1 = raw.find(' ');
    if (pos1 == std::string::npos)
        return req;
    req.method = raw.substr(0, pos1);

    size_t pos2 = raw.find(' ', pos1 + 1);
    if (pos2 == std::string::npos)
        return req;
    req.path = raw.substr(pos1 + 1, pos2 - pos1 - 1);
    size_t body_pos = raw.find("\r\n\r\n");
    if (body_pos != std::string::npos)
    {
        req.body = raw.substr(body_pos + 4);
    }
    if (!req.path.empty() && req.path[req.path.size() - 1] == '\r')
        req.path.resize(req.path.size() - 1);

    if (!req.body.empty())
    {
        while (req.body.size() > 0 && 
              (req.body[req.body.size() - 1] == '\r' || req.body[req.body.size() - 1] == '\n'))
        {
            req.body.resize(req.body.size() - 1);
        }
    }
    return req;
}
