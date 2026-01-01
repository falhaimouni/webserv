#include "../../includes/handlers/RequestHandler.hpp"
#include "../../includes/handlers/StaticHandler.hpp"
#include "../../includes/handlers/UploadHandler.hpp"
#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include "../../includes/handlers/RequestRouter.hpp"

Response RequestHandler::handleRequest(const std::string &rawRequest)
{
    // Parse request into a Request object
    Request req = RequestParser::parse(rawRequest);

    // Route request to the right handler
    return routeRequest(req);
}
