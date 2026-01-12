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
    static std::string parseMultipartFilename(const std::string &body, const std::string &boundary);
    static std::string parseMultipartContent(const std::string &body, const std::string &boundary);
    static std::string sanitizeFilename(const std::string &filename);
    static bool isPathTraversal(const std::string &path);
    static std::string getBoundaryFromContentType(const std::string &contentType);
    
public:
    static Response handleUploadReq(const std::string &file, const std::string &data);
    static Response handleUploadReq(const Request &req, const LocationConfig &config, size_t maxBodySize);
};

#endif