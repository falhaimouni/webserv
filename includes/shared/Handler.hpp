#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "Request.hpp"
#include "LocationConfig.hpp"
#include "Response.hpp"

class Handler {
public:
    virtual ~Handler() {}
    virtual Response handle(const Request& req,
                            const LocationConfig& loc) = 0;
};

#endif

