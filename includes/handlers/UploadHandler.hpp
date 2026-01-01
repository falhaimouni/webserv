#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include "../shared/Response.hpp"

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

class UploadHandler
{
private:
    
public:
    static Response handleUploadReq(const std::string &file, const std::string &data);
};

#endif