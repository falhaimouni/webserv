#include "../../includes/handlers/RequestRouter.hpp"
#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include "../../includes/handlers/DirectoryHandler.hpp"
#include "../../includes/shared/LocationConfig.hpp"
#include <sys/stat.h>
#include <unistd.h>

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

static LocationConfig createDefaultConfig()
{
    LocationConfig config;
    config.path = "/";
    config.root = "./www";
    config.index = "index.html";
    config.autoindex = true;
    config.upload_store = "./uploads";
    config.is_cgi = false;
    config.methods.insert("GET");
    config.methods.insert("POST");
    config.methods.insert("DELETE");
    config.cgi_ext.insert(".py");
    config.cgi_ext.insert(".php");
    config.cgi_ext.insert(".pl");
    config.cgi_ext.insert(".sh");
    return config;
}

Response routeRequest(const Request &req)
{
    LocationConfig config = createDefaultConfig();
    return routeRequest(req, config);
}

Response routeRequest(const Request &req, const LocationConfig &config)
{
    // Check if method is allowed
    if (config.methods.find(req.method) == config.methods.end())
    {
        return ErrorHandler::handleError(405, "Method Not Allowed");
    }

    // Handle uploads immediately; the upload handler manages storage path
    // and does not require the target file to already exist on disk.
    if (req.method == "POST" || req.method == "PUT")
    {
        // Default max body size: 1MB
        size_t maxBodySize = 1024 * 1024;
        return UploadHandler::handleUploadReq(req, config, maxBodySize);
    }
    
    // Resolve full path
    std::string fullPath = resolvePath(req.path, config.root);
    
    // Check if path exists
    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
    {
        return ErrorHandler::handleError(404, "Not Found");
    }
    
    // Check if it's a CGI script
    if (config.is_cgi || S_ISREG(st.st_mode))
    {
        // Check if file extension matches CGI extensions
        size_t dotPos = fullPath.rfind('.');
        if (dotPos != std::string::npos)
        {
            std::string ext = fullPath.substr(dotPos);
            if (config.cgi_ext.find(ext) != config.cgi_ext.end())
            {
                return CGIHandler::handleCGIReq(req, config, fullPath);
            }
        }
    }
    
    // Handle directory
    if (S_ISDIR(st.st_mode))
    {
        if (req.method == "GET")
        {
            // Try to find index file first
            std::string indexPath = fullPath;
            if (indexPath[indexPath.size() - 1] != '/')
                indexPath += "/";
            indexPath += config.index;
            
            struct stat indexSt;
            if (stat(indexPath.c_str(), &indexSt) == 0 && S_ISREG(indexSt.st_mode))
            {
                return StaticHandler::handleStaticReq(req, config);
            }
            
            // No index file, check autoindex
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
    
    // Handle regular file
    if (S_ISREG(st.st_mode))
    {
        if (req.method == "GET")
        {
            return StaticHandler::handleStaticReq(req, config);
        }
        else if (req.method == "DELETE")
        {
            // Delete the resolved file
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
