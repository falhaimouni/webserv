#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include "../shared/Response.hpp"

class ErrorHandler
{
private:
    
public:
    static Response handleError(int status, const std::string &reason);
};

#endif