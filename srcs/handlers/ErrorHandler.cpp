#include "../../includes/handlers/ErrorHandler.hpp"
#include "../../includes/shared/LocationConfig.hpp"
#include <sstream>
#include <fcntl.h>
#include <unistd.h>

std::string ErrorHandler::getErrorPagePath(int status, const LocationConfig &config)
{
    std::map<int, std::string>::const_iterator it = config.error_pages.find(status);
    if (it != config.error_pages.end())
    {
        // If error page path starts with ./ or is absolute, use it as-is
        // Otherwise, prepend root
        const std::string &errorPath = it->second;
        if (errorPath.size() >= 2 && errorPath[0] == '.' && errorPath[1] == '/')
            return errorPath;
        if (!errorPath.empty() && errorPath[0] == '/')
        {
            // Absolute path from root
            std::string result = config.root;
            if (!result.empty() && result[result.size() - 1] != '/')
                result += "/";
            result += errorPath.substr(1); // Skip leading /
            return result;
        }
        // Relative path - prepend root
        return config.root + "/" + errorPath;
    }
    return "";
}

Response ErrorHandler::handleError(int status, const std::string &reason, const LocationConfig &config)
{
    std::string customPagePath = getErrorPagePath(status, config);
    if (!customPagePath.empty())
        return handleError(status, reason, customPagePath); // Use custom page if available

    return handleError(status, reason);
}

Response ErrorHandler::handleError(int status, const std::string &reason)
{
    Response res;
    res.status = status;
    res.reason = reason;
    res.headers["Content-Type"] = "text/html";

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

    int fd = open(customPagePath.c_str(), O_RDONLY);
    if (fd == -1)
        return handleError(status, reason);

    std::string content;
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
        content.append(buffer, bytesRead);

    close(fd);

    res.body = content;
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();

    return res;
}
