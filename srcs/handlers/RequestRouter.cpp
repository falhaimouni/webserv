#include "../../includes/handlers/RequestRouter.hpp"
#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"

Response routeRequest(const Request &req)
{
    if (req.path == "/static")
    {
        return StaticHandler::handleStaticReq("index.html");
    }
    else if (req.path == "/upload")
    {
        return UploadHandler::handleUploadReq("upload_test.txt", "Hello Upload!");
    }
    else if (req.path == "/cgi-bin")
    {
        return CGIHandler::handleCGIReq("test_script.txt", "input_data");
    }
    else
    {
        return ErrorHandler::handleError(404, "Not Found");
    }
}
