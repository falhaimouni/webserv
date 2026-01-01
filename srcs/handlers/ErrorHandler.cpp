#include "../../includes/handlers/ErrorHandler.hpp"

Response ErrorHandler::handleError(int status, const std::string &reason)
{
    Response res;
    std::stringstream ss;
    ss << status;
    res.status = status;
    res.reason = reason;
    res.headers["Content-Type"] = "text/html";
    res.body = "<html><body><h1>" + ss.str() + " " + reason + "</h1></body></html>";
    
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}