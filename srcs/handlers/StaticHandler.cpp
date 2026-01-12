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
    // Check if path contains ".." which could escape root directory
    if (path.find("..") != std::string::npos)
        return true;
    
    // Resolve absolute paths and check if they're within root
    char resolvedPath[PATH_MAX];
    char resolvedRoot[PATH_MAX];
    
    if (realpath(path.c_str(), resolvedPath) == NULL)
        return true;
    if (realpath(root.c_str(), resolvedRoot) == NULL)
        return true;
    
    std::string resolvedPathStr(resolvedPath);
    std::string resolvedRootStr(resolvedRoot);
    
    // Check if resolved path starts with resolved root
    if (resolvedPathStr.find(resolvedRootStr) != 0)
        return true;
    
    return false;
}

std::string StaticHandler::resolvePath(const std::string &requestPath, const std::string &root)
{
    std::string resolved = root;
    
    // Remove leading slash from request path
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
    
    const size_t BUFFER_SIZE = 65536;  // Increased from 4096 for better throughput
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
    // Resolve full path
    std::string fullPath = resolvePath(req.path, config.root);
    
    // Security check: prevent path traversal
    if (isPathTraversal(fullPath, config.root))
    {
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    // Check if path exists
    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
    {
        return ErrorHandler::handleError(404, "Not Found");
    }
    
    // If it's a directory, check for index file or autoindex
    if (S_ISDIR(st.st_mode))
    {
        // Try to find index file
        std::string indexPath = findIndexFile(fullPath, config.index);
        if (!indexPath.empty())
        {
            // Serve index file
            return handleStaticReq(indexPath);
        }
        
        // If no index file and autoindex is enabled, show directory listing
        if (config.autoindex)
        {
            return DirectoryHandler::handle(fullPath, req.path);
        }
        
        // No index file and autoindex disabled
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    // It's a regular file, serve it
    return handleStaticReq(fullPath);
}
