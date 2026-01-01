#ifndef DIRECTORY_HANDLER_HPP
#define DIRECTORY_HANDLER_HPP

#include <string>
#include "../shared/Response.hpp"

class DirectoryHandler
{
public:
    static Response handle(const std::string &path, const std::string &uri);
};

#endif
