#ifndef REQUESTROUTER_HPP
#define REQUESTROUTER_HPP

#include "StaticHandler.hpp"
#include "UploadHandler.hpp"
#include "CGIHandler.hpp"
#include "ErrorHandler.hpp"
#include "ResponseBuilder.hpp"
#include "RequestParser.hpp"
#include "../shared/Response.hpp"
#include "../shared/LocationConfig.hpp"

Response routeRequest(const Request &req);
Response routeRequest(const Request &req, const LocationConfig &config);

#endif
