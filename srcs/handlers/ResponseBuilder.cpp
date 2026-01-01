#include "../../includes/handlers/ResponseBuilder.hpp"

std::string ResponseBuilder::formatResponse(Response &response)
{
    std::ostringstream oss;

    oss << "HTTP/1.0 " << response.status << " " << response.reason << "\r\n";
    for (std::map<std::string, std::string>::iterator it = response.headers.begin();
         it != response.headers.end(); ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }
    oss << "\r\n";
    oss << response.body;

    return oss.str();
}