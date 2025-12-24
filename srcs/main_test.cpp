// #include <iostream>
// #include "handlers.hpp"
// #include "response.hpp"


// // Fake request + location for testing Developer C code only

// int main() {
//     HttpRequest req;
//     LocationConfig loc;

//     // Test request path
//     req.method = "GET";
//     req.path = "./";   // current directory
//     req.body = "";

//     loc.root = ".";
//     loc.autoindex = true;
//     loc.index = ""; // leave empty to force autoindex

//     Response r = Handlers::handleGET(req, loc);

//     std::cout << r.toString();

//     return 0;
// }

#include "handlers.hpp"
#include "http_request.hpp"
#include "server_config.hpp"
#include "response.hpp"

#include <iostream>
#include <sys/stat.h>

int main()
{
    // ---------- Prepare fake request ----------
    HttpRequest req;
    req.method = "POST";
    req.path = "/upload";
    req.body = "Hello from POST upload test!\n";

    // ---------- Prepare location config ----------
    LocationConfig loc;
    loc.upload_dir = "./uploads";
    loc.client_max_body_size = 1024 * 1024; // 1MB

    // ---------- Ensure upload directory exists ----------
    struct stat st;
    if (stat(loc.upload_dir.c_str(), &st) != 0)
    {
        std::cout << "Creating upload directory...\n";
        mkdir(loc.upload_dir.c_str(), 0755);
    }

    // ---------- Call POST handler ----------
    Response res = Handlers::handlePOST(req, loc);

    // ---------- Print HTTP response ----------
    std::cout << res.toString() << std::endl;

    return 0;
}
