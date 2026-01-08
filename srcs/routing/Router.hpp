#ifndef ROUTER_HPP
# define ROUTER_HPP

#include "includes/shared/LocationConfig.hpp"
#include "includes/shared/Request.hpp"
#include <vector>

//location decides behavior, is it static, cgi, upload, which methods allowed

class Router
{
public:
    const LocationConfig* route(const Request& req,
        const std::vector<LocationConfig>& locations); //which location matches the request, compare req path & location path
};


#endif