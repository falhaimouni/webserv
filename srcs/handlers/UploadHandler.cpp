#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include <limits.h>
#include <cstdlib>
#include <algorithm>
#include <sys/stat.h>
#include <iostream>
#include <errno.h>

std::string UploadHandler::getBoundaryFromContentType(const std::string &contentType)
{
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos)
        return "";
    
    boundaryPos += 9; // length of "boundary="
    std::string boundary = contentType.substr(boundaryPos);
    
    // Remove quotes if present
    if (!boundary.empty() && boundary[0] == '"')
    {
        boundary = boundary.substr(1);
        if (!boundary.empty() && boundary[boundary.size() - 1] == '"')
            boundary = boundary.substr(0, boundary.size() - 1);
    }
    
    return boundary;
}

std::string UploadHandler::parseMultipartFilename(const std::string &body, const std::string &boundary)
{
    (void)boundary; // Boundary is used implicitly in the search pattern
    std::string searchStr = "Content-Disposition: form-data;";
    size_t pos = body.find(searchStr);
    if (pos == std::string::npos)
        return "";
    
    size_t namePos = body.find("filename=\"", pos);
    if (namePos == std::string::npos)
    {
        namePos = body.find("filename=", pos);
        if (namePos == std::string::npos)
            return "";
        namePos += 9;
    }
    else
    {
        namePos += 10;
    }
    
    size_t endPos = body.find("\"", namePos);
    if (endPos == std::string::npos)
    {
        // Try without quotes
        endPos = body.find("\r\n", namePos);
        if (endPos == std::string::npos)
            endPos = body.find("\n", namePos);
        if (endPos == std::string::npos)
            return "";
    }
    
    return body.substr(namePos, endPos - namePos);
}

std::string UploadHandler::parseMultipartContent(const std::string &body, const std::string &boundary)
{
    std::string boundaryMarker = "--" + boundary;
    size_t firstBoundary = body.find(boundaryMarker);
    if (firstBoundary == std::string::npos)
        return "";
    
    // Find the start of content (after headers)
    size_t headerEnd = body.find("\r\n\r\n", firstBoundary);
    if (headerEnd != std::string::npos)
    {
        headerEnd += 4; // Skip CRLF CRLF
    }
    else
    {
        headerEnd = body.find("\n\n", firstBoundary);
        if (headerEnd == std::string::npos)
            return "";
        headerEnd += 2; // Skip LF LF
    }
    
    // Find the end of content (next boundary or end of body)
    size_t nextBoundary = body.find(boundaryMarker, headerEnd);
    if (nextBoundary == std::string::npos)
        nextBoundary = body.size();
    
    // Remove trailing CRLF before boundary
    while (nextBoundary > headerEnd && 
           (body[nextBoundary - 1] == '\n' || body[nextBoundary - 1] == '\r'))
        nextBoundary--;
    
    return body.substr(headerEnd, nextBoundary - headerEnd);
}

std::string UploadHandler::sanitizeFilename(const std::string &filename)
{
    std::string sanitized = filename;
    
    // Remove path components
    size_t lastSlash = sanitized.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        sanitized = sanitized.substr(lastSlash + 1);
    
    // Remove dangerous characters
    for (size_t i = 0; i < sanitized.size(); ++i)
    {
        char c = sanitized[i];
        if (c == '\0' || c == '/' || c == '\\' || c == ':' || 
            c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
        {
            sanitized[i] = '_';
        }
    }
    
    return sanitized;
}

bool UploadHandler::isPathTraversal(const std::string &path)
{
    return path.find("..") != std::string::npos;
}

Response UploadHandler::handleUploadReq(const Request &req, const LocationConfig &config, size_t maxBodySize)
{
    // Check if upload_store is configured
    if (config.upload_store.empty())
    {
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    // Check body size
    if (req.body.size() > maxBodySize)
    {
        return ErrorHandler::handleError(413, "Request Entity Too Large");
    }
    
    std::string filename;
    std::string content;
    
    // Check Content-Type (headers stored lowercase)
    std::map<std::string, std::string>::const_iterator contentTypeIt = req.headers.find("content-type");
    if (contentTypeIt != req.headers.end())
    {
        std::string contentType = contentTypeIt->second;
        
        // Handle multipart/form-data
        if (contentType.find("multipart/form-data") != std::string::npos)
        {
            std::string boundary = getBoundaryFromContentType(contentType);
            if (boundary.empty())
            {
                return ErrorHandler::handleError(400, "Bad Request");
            }
            
            filename = parseMultipartFilename(req.body, boundary);
            content = parseMultipartContent(req.body, boundary);
        }
        else
        {
            // For other content types, use path as filename or generate one
            size_t lastSlash = req.path.find_last_of('/');
            if (lastSlash != std::string::npos && lastSlash + 1 < req.path.size())
            {
                filename = req.path.substr(lastSlash + 1);
            }
            else
            {
                filename = "uploaded_file";
            }
            content = req.body;
        }
    }
    else
    {
        // No Content-Type, use path as filename
        size_t lastSlash = req.path.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < req.path.size())
        {
            filename = req.path.substr(lastSlash + 1);
        }
        else
        {
            filename = "uploaded_file";
        }
        content = req.body;
    }
    
    // Sanitize filename
    filename = sanitizeFilename(filename);
    if (filename.empty())
    {
        filename = "uploaded_file";
    }
    
    // Check for path traversal in filename
    if (isPathTraversal(filename))
    {
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    // Build full upload path
    std::string uploadPath = config.upload_store;
    if (uploadPath[uploadPath.size() - 1] != '/')
        uploadPath += "/";
    uploadPath += filename;
    
    // Create upload directory if it doesn't exist (do this BEFORE realpath checks)
    struct stat dirStat;
    if (stat(config.upload_store.c_str(), &dirStat) != 0)
    {
        // Directory doesn't exist, create it
        if (mkdir(config.upload_store.c_str(), 0755) != 0)
        {
            std::cerr << "Failed to create upload directory: " << config.upload_store 
                      << " (errno: " << errno << ")" << std::endl;
            return ErrorHandler::handleError(500, "Internal Server Error");
        }
    }
    else if (!S_ISDIR(dirStat.st_mode))
    {
        std::cerr << "Upload store exists but is not a directory: " << config.upload_store << std::endl;
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    char resolvedStore[PATH_MAX];
    if (realpath(config.upload_store.c_str(), resolvedStore) == NULL)
    {
        std::cerr << "Failed to resolve upload_store: " << config.upload_store 
                  << " (errno: " << errno << ")" << std::endl;
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    std::string resolvedStoreStr(resolvedStore);
    if (resolvedStoreStr[resolvedStoreStr.size() - 1] != '/')
        resolvedStoreStr += "/";
    std::string fullResolvedPath = resolvedStoreStr + filename;
    
    if (fullResolvedPath.find(resolvedStoreStr) != 0)
    {
        std::cerr << "Path traversal detected in upload" << std::endl;
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    ssize_t written = write(fd, content.c_str(), content.size());
    close(fd);
    
    if (written < 0 || static_cast<size_t>(written) != content.size())
    {
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    Response res;
    res.status = 201;
    res.reason = "Created";
    res.headers["Content-Type"] = "text/html";
    res.headers["Location"] = req.path;
    
    std::ostringstream body;
    body << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head><title>201 Created</title></head>\n"
         << "<body>\n"
         << "    <h1>201 Created</h1>\n"
         << "    <p>File '" << filename << "' uploaded successfully.</p>\n"
         << "</body>\n"
         << "</html>\n";
    
    res.body = body.str();
    
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}