#include "../../includes/handlers/RequestRouter.hpp"
#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include "../../includes/handlers/DirectoryHandler.hpp"

#include <sys/stat.h>

// static bool isDirectory(const std::string &path)
// {
//     struct stat st;
//     if (stat(path.c_str(), &st) != 0)
//         return false;
//     return S_ISDIR(st.st_mode);
// }

static std::string resolvePath(const std::string &uri)
{
    return "./www" + uri;
}

Response routeRequest(const Request &req)
{
    std::string fullPath = resolvePath(req.path);

    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
        return ErrorHandler::handleError(404, "Not Found");

    bool autoindex = true; // 🔧 later from config

    if (S_ISDIR(st.st_mode))
    {
        if (!autoindex)
            return ErrorHandler::handleError(403, "Autoindex Off");

        return DirectoryHandler::handle(fullPath, req.path);
    }

    if (req.method == "GET")
        return StaticHandler::handleStaticReq(fullPath);

    if (req.method == "POST")
        return UploadHandler::handleUploadReq(fullPath, req.body);

    return ErrorHandler::handleError(405, "Method Not Allowed");
}
