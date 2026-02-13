#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include "../shared/Response.hpp"
#include "../shared/LocationConfig.hpp"
#include <string>

class ErrorHandler
{
private:
    static std::string getErrorPagePath(int status, const LocationConfig &config);
    
public:
    static Response handleError(int status, const std::string &reason);
    static Response handleError(int status, const std::string &reason, const LocationConfig &config);
    static Response handleError(int status, const std::string &reason, const std::string &customPagePath);
};

#endif