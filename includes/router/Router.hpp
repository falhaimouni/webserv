#ifndef ROUTER_HPP
# define ROUTER_HPP

#include "../shared/LocationConfig.hpp"
#include "../shared/Request.hpp"
#include "../shared/HttpStatus.hpp"
#include <vector>

//location decides behavior, is it static, cgi, upload, which methods allowed

class Router
{
public:
    const LocationConfig* route(const Request& req,
        const std::vector<LocationConfig>& locations, int& status); //which location matches the request, compare req path & location path

private:
        //no match = 0/ match >0 (longest wins)
        size_t matchLocation (const LocationConfig& loc, const Request& req);
        bool matchMethod (const LocationConfig& loc, const Request& req);
};

#endif