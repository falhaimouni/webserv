#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include "../shared/Response.hpp"

#include <string>
#include <sstream>

class ResponseBuilder
{
private:
    static std::string getCurrentDate();
    static std::string getServerName();
   
public:
    static std::string formatResponse(Response &response, const std::string &sessionID = "");
    static std::string formatResponse(Response &response, const std::string &httpVersion, const std::string &sessionID = "");
};


#endif