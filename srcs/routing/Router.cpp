#include "../../includes/router/Router.hpp"

const LocationConfig* Router::route(const Request& req,
        const std::vector<LocationConfig>& locations, int& status)
{
    if (locations.empty())
    {
        status = HTTP_NOT_FOUND; //404
        return NULL;
    }

    size_t bestLength = 0; //keep track of longest
    const LocationConfig* bestLocation = NULL; //pointer to the best match

    for (size_t i = 0; i < locations.size(); i++)
    {
        const LocationConfig& loc = locations[i]; //each individual location
        size_t len = matchLocation(loc, req);

        if (len >  bestLength)
        {
            bestLength = len;
            bestLocation = &loc;
        }
    }
    //no match at all //return 404
    if (bestLocation == NULL)
    {
        status = HTTP_NOT_FOUND;
        return NULL;
    }

    if (!bestLocation->redirect_to.empty())
    {
        status = HTTP_MOVED_PERMANENTLY; //301
        return bestLocation;
    }

    //return 405
    if (!matchMethod(*bestLocation, req))
    {
        status = HTTP_METHOD_NOT_ALLOWED;
        return NULL;
    }
    //best match with allowes method
    status = HTTP_OK;
    return bestLocation;
}

size_t Router::matchLocation(const LocationConfig& loc, const Request& req)
{
    const std::string& locPath = loc.path;
    const std::string& reqPath = req.path;

    //match everything
    if (locPath == "/")
        return 1;
    //req path must start with loc path
    if (reqPath.compare(0, locPath.length(), locPath) != 0)
        return 0;
    // so /upload isin't /uploads
    //give me the character right after in the req path
    if (reqPath.length() > locPath.length() && reqPath[locPath.length()] != '/')
        return 0;
    return locPath.length(); //longest
}

bool Router::matchMethod(const LocationConfig& loc, const Request& req)
{
    //empty = all methods allowed
    if (loc.methods.empty())
        return true;
    //iterator cuz methods is set
    if (loc.methods.find(req.method) != loc.methods.end())
        return true;
    return false;
}