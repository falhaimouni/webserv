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

static std::string stripQuery(const std::string &path)
{
    size_t qpos = path.find('?');
    if (qpos == std::string::npos)
        return path;
    return path.substr(0, qpos);
}

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

static std::string buildLocationRelativePath(const std::string &reqPath,
                                             const std::string &locPath)
{
    if (locPath == "/")
        return reqPath;

    if (reqPath.compare(0, locPath.size(), locPath) != 0)
        return reqPath;

    std::string rel = reqPath.substr(locPath.size());
    if (!rel.empty() && rel[0] == '/')
        rel = rel.substr(1);
    return rel;
}

// Check if the resolved path is within the root directory (path traversal protection)
static bool isPathSafe(const std::string &resolvedPath, const std::string &root)
{
    char realRoot[PATH_MAX];
    
    // Get canonical root (must exist)
    if (!realpath(root.c_str(), realRoot))
        return false;  // Root path doesn't exist - this should never happen in normal operation
    
    std::string rootStr = std::string(realRoot);
    if (rootStr[rootStr.size() - 1] != '/')
        rootStr += "/";
    
    // Try to get canonical resolved path
    char realResolved[PATH_MAX];
    if (realpath(resolvedPath.c_str(), realResolved))
    {
        // File exists - use its canonical path
        std::string resolvedStr = std::string(realResolved);
        
        // Check if it starts with root
        if (resolvedStr.compare(0, rootStr.size() - 1, rootStr.substr(0, rootStr.size() - 1)) == 0)
            return true;
        
        return false;
    }
    
    // File doesn't exist - check if its parent exists and if the path would be under root
    std::string parentPath = resolvedPath;
    size_t lastSlash = parentPath.rfind('/');
    if (lastSlash != std::string::npos)
        parentPath = parentPath.substr(0, lastSlash);
    
    // If parentPath is empty, use current directory
    if (parentPath.empty())
        parentPath = ".";
    
    if (!realpath(parentPath.c_str(), realResolved))
        return false;  // Parent doesn't exist either
    
    std::string parentRealPath = std::string(realResolved);
    if (parentRealPath[parentRealPath.size() - 1] != '/')
        parentRealPath += "/";
    
    // Check if parent starts with root (file would be under root)
    std::string rootNoSlash = rootStr.substr(0, rootStr.size() - 1);
    if (parentRealPath.find(rootNoSlash + "/") == 0 || parentRealPath == rootNoSlash + "/")
        return true;
    
    return false;
}

RouteResult routeRequest(const Request &req, const LocationConfig &config)
{
    RouteResult result;
    // Block path traversal attempts
    if (req.path.find("..") != std::string::npos)
    {
        result.response = ErrorHandler::handleError(403, "Forbidden", config);
        return result;
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
        result.response = res;
        return result;
    }
    
    // Check method restrictions BEFORE processing uploads or any other handler
    if (!config.methods.empty() && config.methods.find(req.method) == config.methods.end())
    {
        result.response = ErrorHandler::handleError(405, "Method Not Allowed", config);
        return result;
    }
    
    // Strip query string from path before resolving
    std::string cleanPath = stripQuery(req.path);

    std::string relativePath = buildLocationRelativePath(cleanPath, config.path);
    
    // Special case: if relative path is empty and location looks like a file (contains dot),
    // append the location filename to the relative path
    if (relativePath.empty() && !config.path.empty() && config.path.find('.') != std::string::npos)
    {
        relativePath = config.path;
        if (!relativePath.empty() && relativePath[0] == '/')
            relativePath = relativePath.substr(1);
    }
    
    // Special case: if relative path is empty and location is a directory path (ends with /),
    // use the location path (without leading slash) as the relative path
    if (relativePath.empty() && !config.path.empty() && config.path[config.path.length() - 1] == '/' && config.path != "/")
    {
        relativePath = config.path;
        if (!relativePath.empty() && relativePath[0] == '/')
            relativePath = relativePath.substr(1);
        // Remove trailing slash
        if (!relativePath.empty() && relativePath[relativePath.length() - 1] == '/')
            relativePath = relativePath.substr(0, relativePath.length() - 1);
    }
    
    std::string fullPath = resolvePath(relativePath, config.root);
    
    // Check for path traversal attacks
    if (!isPathSafe(fullPath, config.root))
    {
        result.response = ErrorHandler::handleError(403, "Forbidden", config);
        return result;
    }
    
    struct stat st;
    bool fileExists = (stat(fullPath.c_str(), &st) == 0);
    
    // **CGI DETECTION FIRST** - check if this is a CGI script before anything else
    // This ensures CGI scripts are handled as CGI, not as file uploads
    if (fileExists && (config.is_cgi || S_ISREG(st.st_mode)))
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
                    result.response = ErrorHandler::handleError(405, "Method Not Allowed", config);
                    return result;
                }
                result.isCgi = true;
                result.cgiPath = fullPath;
                return result;
            }
        }
    }
    
    // **UPLOAD HANDLER** - allow POST to non-existent paths for file creation
    if (req.method == "POST")
    {
        if (config.upload_enable)
        {
            size_t maxBodySize = config.max_body_size;
            result.response = UploadHandler::handleUploadReq(req, config, maxBodySize);
            return result;
        }
    }
    
    // File must exist for non-upload requests
    if (!fileExists)
    {
        result.response = ErrorHandler::handleError(404, "Not Found", config);
        return result;
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
                // Call the simple handleStaticReq with the already-resolved path
                result.response = StaticHandler::handleStaticReq(indexPath);
                return result;
            }
            
            if (config.autoindex)
            {
                result.response = DirectoryHandler::handle(fullPath, req.path);
                return result;
            }

            result.response = ErrorHandler::handleError(403, "Forbidden", config);
            return result;
        }
        else
        {
            result.response = ErrorHandler::handleError(405, "Method Not Allowed", config);
            return result;
        }
    }
    
    if (S_ISREG(st.st_mode))
    {
        if (req.method == "GET")
        {
            // Call the simple handleStaticReq with the already-resolved path
            result.response = StaticHandler::handleStaticReq(fullPath);
            return result;
        }
        else if (req.method == "DELETE")
        {
            if (unlink(fullPath.c_str()) == 0)
            {
                Response res;
                res.status = 204;
                res.reason = "No Content";
                res.headers["Content-Length"] = "0";
                result.response = res;
                return result;
            }
            result.response = ErrorHandler::handleError(500, "Internal Server Error", config);
            return result;
        }
    }

    result.response = ErrorHandler::handleError(500, "Internal Server Error", config);
    return result;
}
