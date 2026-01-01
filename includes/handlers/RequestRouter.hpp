#ifndef REQUESTROUTER_HPP
#define REQUESTROUTER_HPP

#include "StaticHandler.hpp"
#include "UploadHandler.hpp"
#include "CGIHandler.hpp"
#include "ErrorHandler.hpp"
#include "ResponseBuilder.hpp"
#include "RequestParser.hpp"
#include "../shared/Response.hpp"

Response routeRequest(const Request &req);

#endif
