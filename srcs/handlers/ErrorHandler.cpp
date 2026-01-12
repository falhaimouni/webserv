#include "../../includes/handlers/ErrorHandler.hpp"
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

std::string ErrorHandler::getErrorPagePath(int status)
{
    // Default error pages location (can be configured later)
    std::ostringstream path;
    path << "./www/error_pages/" << status << ".html";
    return path.str();
}

Response ErrorHandler::handleError(int status, const std::string &reason)
{
    // Try to load custom error page first
    std::string customPagePath = getErrorPagePath(status);
    int fd = open(customPagePath.c_str(), O_RDONLY);
    if (fd != -1)
    {
        close(fd);
        return handleError(status, reason, customPagePath);
    }
    
    // Fall back to default error page
    Response res;
    std::stringstream ss;
    ss << status;
    res.status = status;
    res.reason = reason;
    res.headers["Content-Type"] = "text/html";
    
    // Generate better HTML error page
    std::ostringstream body;
    body << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <title>" << status << " " << reason << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background-color: #f5f5f5; }\n"
         << "        .error-container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); display: inline-block; }\n"
         << "        h1 { color: #d32f2f; font-size: 72px; margin: 0; }\n"
         << "        h2 { color: #333; margin: 10px 0; }\n"
         << "        p { color: #666; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"error-container\">\n"
         << "        <h1>" << status << "</h1>\n"
         << "        <h2>" << reason << "</h2>\n"
         << "        <p>The requested resource could not be found or accessed.</p>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>\n";
    
    res.body = body.str();
    
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}

Response ErrorHandler::handleError(int status, const std::string &reason, const std::string &customPagePath)
{
    Response res;
    res.status = status;
    res.reason = reason;
    res.headers["Content-Type"] = "text/html";
    
    // Read custom error page
    int fd = open(customPagePath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        // If custom page can't be read, fall back to default
        return handleError(status, reason);
    }
    
    char buffer[4096];
    std::string content;
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        content.append(buffer, bytesRead);
    }
    close(fd);
    
    res.body = content;
    
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}