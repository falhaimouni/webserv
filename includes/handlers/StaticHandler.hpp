#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "../shared/Response.hpp"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

class StaticHandler
{
private:
    
public:
    static Response handleStaticReq(const std::string &reqPath);
};


#endif