#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "../shared/Response.hpp"
#include "../shared/Request.hpp"
#include "../shared/LocationConfig.hpp"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <vector>

class StaticHandler
{
private:
    static bool isPathTraversal(const std::string &path, const std::string &root);
    static std::string resolvePath(const std::string &requestPath, const std::string &root);
    static std::string findIndexFile(const std::string &dirPath, const std::string &index);
    static bool isDirectory(const std::string &path);
    
public:
    static Response handleStaticReq(const std::string &reqPath);
    static Response handleStaticReq(const Request &req, const LocationConfig &config);
};


#endif