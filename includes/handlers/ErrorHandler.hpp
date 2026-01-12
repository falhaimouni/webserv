#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include "../shared/Response.hpp"
#include <string>

class ErrorHandler
{
private:
    static std::string getErrorPagePath(int status);
    
public:
    static Response handleError(int status, const std::string &reason);
    static Response handleError(int status, const std::string &reason, const std::string &customPagePath);
};

#endif