#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/getType.hpp"

Response StaticHandler::handleStaticReq(const std::string &reqPath)
{
    Response res;

    int fd = open(reqPath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        res.status = 404;
        res.reason = "Not Found";
        res.headers["Content-Type"] = "text/html";
        res.body = "...";
    }
    else
    {
        res.status = 200;
        res.reason = "OK";
        
        char buffer[1024];
        std::string content;
        ssize_t bytesRead;
        while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
        content.append(buffer, bytesRead);
        
        res.body = content;
        res.headers["Content-Type"] = getType::getMimeType(reqPath);
        close(fd);
    }
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}
