#include "../../includes/handlers/DirectoryHandler.hpp"
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>

Response DirectoryHandler::handle(const std::string &path, const std::string &uri)
{
    Response res;
    std::ostringstream body;

    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        res.status = 500;
        res.reason = "Internal Server Error";
        res.body = "<h1>500 Internal Server Error</h1>";
        return res;
    }

    body << "<html><head><title>Index of " << uri << "</title></head>";
    body << "<body><h1>Index of " << uri << "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;

        body << "<li><a href=\"" << uri;
        if (uri[uri.size() - 1] != '/')
            body << "/";
        body << name << "\">" << name << "</a></li>";
    }

    body << "</ul></body></html>";
    closedir(dir);

    res.status = 200;
    res.reason = "OK";
    res.body = body.str();
    res.headers["Content-Type"] = "text/html";
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();

    return res;
}
