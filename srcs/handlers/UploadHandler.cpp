#include "../../includes/handlers/UploadHandler.hpp"

Response UploadHandler::handleUploadReq(const std::string &file, const std::string &data)
{
    Response res;
    int fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        res.status = 500;
        res.reason = "Internal Server Error";
        res.headers["Content-Type"] = "text/html";
        res.body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    }
    else
    {
        write(fd, data.c_str(), data.size());
        close(fd);
        res.status = 201;
        res.reason = "Created";
        res.headers["Content-Type"] = "text/html";
        res.body = "<html><body><h1>201 Created</h1></body></html>";
    }    
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}