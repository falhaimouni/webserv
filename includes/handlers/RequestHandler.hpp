#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "ResponseBuilder.hpp"
#include "RequestParser.hpp"

class RequestHandler
{
public:
    static Response handleRequest(const std::string &rawRequest);
};

#endif
