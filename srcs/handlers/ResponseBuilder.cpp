#include "../../includes/handlers/ResponseBuilder.hpp"
#include <ctime>
#include <cstring>

std::string ResponseBuilder::getCurrentDate()
{
    time_t now = time(NULL);
    struct tm *timeinfo = gmtime(&now);
    char buffer[128];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
    return std::string(buffer);
}

std::string ResponseBuilder::getServerName()
{
    return "webserv/1.0";
}
std::string ResponseBuilder::formatResponse(Response &response, const std::string &sessionID)
{
    return formatResponse(response, "HTTP/1.0", sessionID);
}

std::string ResponseBuilder::formatResponse(Response &response,
                                            const std::string &httpVersion,
                                            const std::string &sessionID)
{
    std::ostringstream oss;

    // Status line
    oss << httpVersion << " " << response.status << " " << response.reason << "\r\n";

    // Automatically attach session cookie
    if (!sessionID.empty())
    {
        response.headers["Set-Cookie"] =
            "session_id=" + sessionID + "; Path=/; HttpOnly";
    }

    // Ensure standard headers are set
    if (response.headers.find("Date") == response.headers.end())
        response.headers["Date"] = getCurrentDate();

    if (response.headers.find("Server") == response.headers.end())
        response.headers["Server"] = getServerName();

    // Set Content-Length
    if (response.headers.find("Content-Length") == response.headers.end())
    {
        std::stringstream len;
        len << response.body.size();
        response.headers["Content-Length"] = len.str();
    }

    // Connection header
    if (response.headers.find("Connection") == response.headers.end())
        response.headers["Connection"] = "close";

    // Write headers
    for (std::map<std::string, std::string>::iterator it = response.headers.begin();
         it != response.headers.end(); ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }

    oss << "\r\n";
    oss << response.body;

    return oss.str();
}
