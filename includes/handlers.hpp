#ifndef HANDLERS_HPP
#define HANDLERS_HPP

#include "http_request.hpp"
#include "response.hpp"
#include "server_config.hpp"

class Handlers {
public:
    // main public entry for Dev C (Day 1: GET only)
    static Response handleGET(const HttpRequest& req, const LocationConfig& loc);
};

#endif
