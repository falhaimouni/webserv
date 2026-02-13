#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include "../shared/Response.hpp"
#include "../shared/Request.hpp"
#include "../shared/LocationConfig.hpp"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>

class UploadHandler
{
private:
    static bool isPathTraversal(const std::string &path);
    static std::string getBoundary(const std::string &contentType);
    static std::string reFormat(const std::string &filename);
    static std::string getFileName(const std::string &body, const std::string &boundary);
    static std::string parseMultipart(const std::string &body, const std::string &boundary);
    
public:
    static Response handleUploadReq(const Request &req, const LocationConfig &config, size_t maxBodySize);
};

#endif