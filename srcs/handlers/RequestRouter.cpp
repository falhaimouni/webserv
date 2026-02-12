#include "../../includes/handlers/RequestRouter.hpp"
#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include "../../includes/handlers/DirectoryHandler.hpp"
#include "../../includes/shared/LocationConfig.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <cstdlib>

static std::string resolvePath(const std::string &uri, const std::string &root)
{
    std::string resolved = root;
    if (resolved[resolved.size() - 1] != '/')
        resolved += "/";
    
    std::string cleanPath = uri;
    if (!cleanPath.empty() && cleanPath[0] == '/')
        cleanPath = cleanPath.substr(1);
    
    if (!cleanPath.empty())
        resolved += cleanPath;
    
    return resolved;
}

// Check if the resolved path is within the root directory (path traversal protection)
static bool isPathSafe(const std::string &resolvedPath, const std::string &root)
{
    char realResolved[PATH_MAX];
    char realRoot[PATH_MAX];
    
    // Try to resolve the actual paths
    const char *resPtr = realpath(resolvedPath.c_str(), realResolved);
    const char *rootPtr = realpath(root.c_str(), realRoot);
    
    // If either doesn't exist, we can't verify - let the normal flow handle it
    if (!resPtr || !rootPtr)
        return true;
    
    std::string resolvedStr(realResolved);
    std::string rootStr(rootPtr);
    
    // Ensure root ends with /
    if (rootStr[rootStr.size() - 1] != '/')
        rootStr += "/";
    
    // Check if resolved path starts with root
    if (resolvedStr.compare(0, rootStr.size(), rootStr) != 0)
    {
        // Also check if it equals root exactly (without trailing slash)
        std::string rootNoSlash = rootStr;
        rootNoSlash = rootNoSlash.substr(0, rootNoSlash.size() - 1);
        if (resolvedStr != rootNoSlash)
            return false;
    }
    
    return true;
}

Response routeRequest(const Request &req, const LocationConfig &config)
{
    // Block path traversal attempts
    if (req.path.find("..") != std::string::npos)
    {
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    if (!config.redirect_to.empty() && config.redirect_code > 0)
    {
        Response res;
        res.status = config.redirect_code;
        res.reason = (config.redirect_code == 301) ? "Moved Permanently" :
                     (config.redirect_code == 302) ? "Found" :
                     (config.redirect_code == 307) ? "Temporary Redirect" : "Redirect";
        res.headers["Location"] = config.redirect_to;
        res.headers["Content-Length"] = "0";
        return res;
    }
    if (config.methods.find(req.method) == config.methods.end())
    {
        return ErrorHandler::handleError(405, "Method Not Allowed");
    }

    if (req.method == "POST" || req.method == "PUT")
    {
        size_t maxBodySize = 1024 * 1024;
        return UploadHandler::handleUploadReq(req, config, maxBodySize);
    }
    
    // Strip query string from path before resolving
    std::string cleanPath = req.path;
    size_t queryPos = cleanPath.find('?');
    if (queryPos != std::string::npos)
        cleanPath = cleanPath.substr(0, queryPos);
    
    std::string fullPath = resolvePath(cleanPath, config.root);
    
    // Check for path traversal attacks
    if (!isPathSafe(fullPath, config.root))
    {
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
    {
        return ErrorHandler::handleError(404, "Not Found");
    }
    
    if (config.is_cgi || S_ISREG(st.st_mode))
    {
        size_t dotPos = fullPath.rfind('.');
        if (dotPos != std::string::npos)
        {
            std::string ext = fullPath.substr(dotPos);
            if (config.cgi_ext.find(ext) != config.cgi_ext.end())
            {
                // CGI scripts only allow GET and POST methods
                if (req.method != "GET" && req.method != "POST")
                {
                    return ErrorHandler::handleError(405, "Method Not Allowed");
                }
                return CGIHandler::handleCGIReq(req, config, fullPath);
            }
        }
    }
    
    if (S_ISDIR(st.st_mode))
    {
        if (req.method == "GET")
        {
            std::string indexPath = fullPath;
            if (indexPath[indexPath.size() - 1] != '/')
                indexPath += "/";
            indexPath += config.index;
            
            struct stat indexSt;
            if (stat(indexPath.c_str(), &indexSt) == 0 && S_ISREG(indexSt.st_mode))
            {
                return StaticHandler::handleStaticReq(req, config);
            }
            
            if (config.autoindex)
            {
                return DirectoryHandler::handle(fullPath, req.path);
            }
            
            return ErrorHandler::handleError(403, "Forbidden");
        }
        else
        {
            return ErrorHandler::handleError(405, "Method Not Allowed");
        }
    }
    
    if (S_ISREG(st.st_mode))
    {
        if (req.method == "GET")
        {
            return StaticHandler::handleStaticReq(req, config);
        }
        else if (req.method == "DELETE")
        {
            if (unlink(fullPath.c_str()) == 0)
            {
                Response res;
                res.status = 204;
                res.reason = "No Content";
                res.headers["Content-Length"] = "0";
                return res;
            }
            return ErrorHandler::handleError(500, "Internal Server Error");
        }
    }
    
    return ErrorHandler::handleError(500, "Internal Server Error");
}
