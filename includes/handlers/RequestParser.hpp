#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "../shared/Request.hpp"
#include <string>

class RequestParser
{
public:
    static Request parse(const std::string &raw);
};

#endif
