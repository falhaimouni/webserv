#include <iostream>
#include "handlers.hpp"
#include "response.hpp"

// Fake request + location for testing Developer C code only

int main() {
    HttpRequest req;
    LocationConfig loc;

    // Test request path
    req.method = "GET";
    req.path = "./";   // current directory
    req.body = "";

    loc.root = ".";
    loc.autoindex = true;
    loc.index = ""; // leave empty to force autoindex

    Response r = Handlers::handleGET(req, loc);

    std::cout << r.toString();

    return 0;
}
