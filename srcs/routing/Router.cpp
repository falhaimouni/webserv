#include "../../includes/router/Router.hpp"

static std::string stripQuery(const std::string &path)
{
    size_t qpos = path.find('?');
    if (qpos == std::string::npos)
        return path;
    return path.substr(0, qpos);
}

const LocationConfig* Router::route(const Request& req,
        const std::vector<LocationConfig>& locations, int& status)
{
    std::string cleanPath = stripQuery(req.path);
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
        Request cleanReq = req;
        cleanReq.path = cleanPath;
        size_t len = matchLocation(loc, cleanReq);

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
    
    // Remove trailing slash from location path for matching
    std::string locPathNoTrail = locPath;
    if (!locPathNoTrail.empty() && locPathNoTrail[locPathNoTrail.length() - 1] == '/')
        locPathNoTrail = locPathNoTrail.substr(0, locPathNoTrail.length() - 1);
    
    // Exact match or request path starts with location path
    if (reqPath == locPathNoTrail)
        return locPathNoTrail.length();
    
    if (reqPath.compare(0, locPathNoTrail.length(), locPathNoTrail) != 0)
        return 0;
    
    // so /upload isn't /uploads - check next character must be / or end of string
    if (reqPath.length() > locPathNoTrail.length() && reqPath[locPathNoTrail.length()] != '/')
        return 0;
    
    return locPathNoTrail.length(); //longest
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