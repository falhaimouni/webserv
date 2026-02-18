#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/getType.hpp"
#include "../../includes/handlers/DirectoryHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include <sys/stat.h>
#include <limits.h>
#include <cstdlib>
#include <iostream>
#include <errno.h>

bool StaticHandler::isPathTraversal(const std::string &path, const std::string &root)
{
    char resolvedPath[PATH_MAX];
    char resolvedRoot[PATH_MAX];

    if (path.find("..") != std::string::npos)
        return true;
    if (realpath(path.c_str(), resolvedPath) == NULL)
        return true;
    if (realpath(root.c_str(), resolvedRoot) == NULL)
        return true;
    
    std::string resolvedPathStr(resolvedPath);
    std::string resolvedRootStr(resolvedRoot);
    if (resolvedPathStr.find(resolvedRootStr) != 0)
        return true;
    return false;
}

std::string StaticHandler::resolvePath(const std::string &requestPath, const std::string &root)
{
    std::string resolved = root;    
    std::string cleanPath = requestPath;
    if (!cleanPath.empty() && cleanPath[0] == '/')
        cleanPath = cleanPath.substr(1);
    if (!cleanPath.empty())
    {
        if (resolved[resolved.size() - 1] != '/')
            resolved += "/";
        resolved += cleanPath;
    }
    return resolved;
}

bool StaticHandler::isDirectory(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

std::string StaticHandler::findIndexFile(const std::string &dirPath, const std::string &index)
{
    if (index.empty())
        return "";
    std::string indexPath = dirPath;
    if (indexPath[indexPath.size() - 1] != '/')
        indexPath += "/";
    indexPath += index;
    struct stat st;
    if (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
        return indexPath;
    return "";
}

Response StaticHandler::handleStaticReq(const std::string &reqPath)
{
    Response res;

    int fd = open(reqPath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        return ErrorHandler::handleError(404, "Not Found");
    }
    
    res.status = 200;
    res.reason = "OK";
    const size_t BUFFER_SIZE = 65536;
    char buffer[BUFFER_SIZE];
    std::string content;
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        if (bytesRead < 0)
        {
            std::cerr << "Error reading file: " << reqPath << std::endl;
            close(fd);
            return ErrorHandler::handleError(500, "Internal Server Error");
        }
        content.append(buffer, bytesRead);
    }
    res.body = content;
    res.headers["Content-Type"] = getType::getMimeType(reqPath);
    close(fd);
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}

Response StaticHandler::handleStaticReq(const Request &req, const LocationConfig &config)
{
    std::string fullPath = resolvePath(req.path, config.root);    
    if (isPathTraversal(fullPath, config.root))
        return ErrorHandler::handleError(403, "Forbidden", config);
    
    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
        return ErrorHandler::handleError(404, "Not Found", config);
    if (S_ISDIR(st.st_mode))
    {
        std::string indexPath = findIndexFile(fullPath, config.index);
        if (!indexPath.empty())
            return handleStaticReq(indexPath);        
        if (config.autoindex)
            return DirectoryHandler::handle(fullPath, req.path);
        return ErrorHandler::handleError(403, "Forbidden", config);
    }
    return handleStaticReq(fullPath);
}
