#ifndef GETTYPE_HPP
#define GETTYPE_HPP

#include "../shared/Response.hpp"

class getType
{
private:
    
public:
    static std::string getMimeType(const std::string &filePath);
};

#endif