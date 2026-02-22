#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include <limits.h>
#include <cstdlib>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <sstream>

std::string UploadHandler::getBoundary(const std::string &contentType)
{
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos)
        return "";
    boundaryPos += 9;
    std::string boundary = contentType.substr(boundaryPos);
    if (!boundary.empty() && boundary[0] == '"')
    {
        boundary = boundary.substr(1);
        if (!boundary.empty() && boundary[boundary.size() - 1] == '"')
            boundary = boundary.substr(0, boundary.size() - 1);
    }
    return boundary;
}

// Extract filename from multipart/form-data
std::string UploadHandler::getFileName(const std::string &body, const std::string &boundary)
{
    (void)boundary;
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
        namePos += 10;
    size_t endPos = body.find("\"", namePos);
    if (endPos == std::string::npos)
    {
        endPos = body.find("\r\n", namePos);
        if (endPos == std::string::npos)
            endPos = body.find("\n", namePos);
        if (endPos == std::string::npos)
            return "";
    }
    return body.substr(namePos, endPos - namePos);
}

// Extract the content of the file from multipart/form-data
std::string UploadHandler::parseMultipart(const std::string &body, const std::string &boundary)
{
    std::string boundaryMarker = "--" + boundary;
    size_t firstBoundary = body.find(boundaryMarker);
    if (firstBoundary == std::string::npos)
        return "";
    size_t headerEnd = body.find("\r\n\r\n", firstBoundary);
    if (headerEnd != std::string::npos)
        headerEnd += 4;
    else
    {
        headerEnd = body.find("\n\n", firstBoundary);
        if (headerEnd == std::string::npos)
            return "";
        headerEnd += 2;
    }
    size_t nextBoundary = body.find(boundaryMarker, headerEnd);
    if (nextBoundary == std::string::npos)
        nextBoundary = body.size();
    while (nextBoundary > headerEnd &&
           (body[nextBoundary - 1] == '\n' || body[nextBoundary - 1] == '\r'))
        nextBoundary--;
    return body.substr(headerEnd, nextBoundary - headerEnd);
}

// Clean filename: remove path traversal and invalid characters
std::string UploadHandler::reFormat(const std::string &filename)
{
    std::string cleanFilename = filename;
    
    size_t lastSlash = cleanFilename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        cleanFilename = cleanFilename.substr(lastSlash + 1);
    for (size_t i = 0; i < cleanFilename.size(); ++i)
    {
        char c = cleanFilename[i];
        if (c == '\0' || c == '/' || c == '\\' || c == ':' || 
            c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
        {
            cleanFilename[i] = '_';
        }
    }    
    return cleanFilename;
}

bool UploadHandler::isPathTraversal(const std::string &path)
{
    return path.find("..") != std::string::npos;
}

Response UploadHandler::handleUploadReq(const Request &req, const LocationConfig &config, size_t maxBodySize)
{
    if (config.upload_store.empty())
        return ErrorHandler::handleError(403, "Forbidden", config);

    if (req.body.size() > maxBodySize)
        return ErrorHandler::handleError(413, "Request Entity Too Large", config);

    if (req.body.empty())
        return ErrorHandler::handleError(400, "Bad Request", config);

    std::string filename;
    std::string content;
    std::string contentType;

    std::map<std::string, std::string>::const_iterator contentTypeIt = req.headers.find("content-type");

    if (contentTypeIt != req.headers.end() && contentTypeIt->second.find("multipart/form-data") != std::string::npos)
    {
        contentType = contentTypeIt->second;
        std::string boundary = getBoundary(contentType);
        if (boundary.empty())
            return ErrorHandler::handleError(400, "Bad Request", config);

        std::string multipartFilename = getFileName(req.body, boundary);
        content = parseMultipart(req.body, boundary);

        size_t lastSlash = req.path.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < req.path.size())
            filename = req.path.substr(lastSlash + 1);
        else if (!multipartFilename.empty())
            filename = multipartFilename;
        else
            filename = "uploaded_file";
    }
    else
    {
        size_t lastSlash = req.path.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < req.path.size())
            filename = req.path.substr(lastSlash + 1);
        else
            filename = "uploaded_file";
        content = req.body;
    }

    if (isPathTraversal(filename))
        return ErrorHandler::handleError(403, "Forbidden", config);

    filename = reFormat(filename);
    if (filename.empty())
        filename = "uploaded_file";

    struct stat dirStat;
    if (stat(config.upload_store.c_str(), &dirStat) != 0)
    {
        if (mkdir(config.upload_store.c_str(), 0755) != 0)
        {
            std::cerr << "Failed to create upload directory: " << config.upload_store
                      << " (errno: " << errno << ")" << std::endl;
            return ErrorHandler::handleError(500, "Internal Server Error", config);
        }
    }
    else if (!S_ISDIR(dirStat.st_mode))
    {
        std::cerr << "Upload store exists but is not a directory: " << config.upload_store << std::endl;
        return ErrorHandler::handleError(500, "Internal Server Error", config);
    }
    std::string uploadPath = config.upload_store;
    if (uploadPath.empty() || uploadPath[uploadPath.size() - 1] != '/')
        uploadPath += "/";
    uploadPath += filename;
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return ErrorHandler::handleError(500, "Internal Server Error", config);

    ssize_t bytes = write(fd, content.c_str(), content.size());
    close(fd);
    if (bytes < 0 || static_cast<size_t>(bytes) != content.size())
        return ErrorHandler::handleError(500, "Internal Server Error", config);
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

    std::ostringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();

    return res;
}
