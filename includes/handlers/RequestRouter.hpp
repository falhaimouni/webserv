#ifndef REQUESTROUTER_HPP
#define REQUESTROUTER_HPP

#include "StaticHandler.hpp"
#include "UploadHandler.hpp"
#include "CGIHandler.hpp"
#include "ErrorHandler.hpp"
#include "ResponseBuilder.hpp"
#include "../shared/Response.hpp"
#include "../shared/LocationConfig.hpp"

struct RouteResult
{
	bool isCgi;
	std::string cgiPath;
	Response response;

	RouteResult() : isCgi(false), cgiPath(""), response() {}
};

RouteResult routeRequest(const Request &req, const LocationConfig &config);

#endif
