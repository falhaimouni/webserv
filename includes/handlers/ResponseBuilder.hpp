#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include "../shared/Response.hpp"

#include <string>
#include <sstream>

class ResponseBuilder
{
private:
   
public:
    static std::string formatResponse(Response &response);
};


#endif