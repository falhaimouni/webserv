#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../shared/Response.hpp"
#include <iostream>
#include <string>

class CGIHandler
{
private:

public:
    static Response handleCGIReq(const std::string &scriptPath, const std::string &input);    
};


#endif