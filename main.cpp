#include <iostream>
#include <string>
#include "includes/shared/Response.hpp"
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

//Recevie request for /index.html
//check if file exists
//if exists, read file, create response with 200 OK
//if not exists, create response with 404 Not Found
//call Response formatter to convert Response struct to string
//send response to socket

#include <iostream>
#include "includes/handlers/StaticHandler.hpp"
#include "includes/handlers/ResponseBuilder.hpp"
#include "includes/handlers/UploadHandler.hpp"
#include "includes/handlers/CGIHandler.hpp"
#include "includes/handlers/ErrorHandler.hpp"
#include "includes/server/Server.hpp"

void call_handler(const std::string &path);
std::string parse_request(const std::string &request);

// int main()
// {
//     std::vector<std::string> requests;
//     requests.push_back("GET /static HTTP/1.0");
//     requests.push_back("GET /upload HTTP/1.0");
//     requests.push_back("GET /cgi-bin HTTP/1.0");
//     requests.push_back("GET /notfound HTTP/1.0");
//     for (size_t i = 0; i < requests.size(); ++i)
//     {
//         std::string path = parse_request(requests[i]); // extract path
//         std::cout << "Requesting: " << path << "\n";
//         call_handler(path); // send to your handler dispatcher
//         std::cout << "-----------------------------------\n";
//     }

//     return 0;
// }


int main()
{
    Server server(8080);
    try {
        server.socketSetup(); // create, bind, listen, non-blocking
    } catch (std::runtime_error &e) {
        std::cerr << "Server setup failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server running on port 8080...\n";

    while (true)
    {
        server.acceptConnections(); // accept clients

        // For each accepted client, read request, parse, route, send response
        // (use your RequestParser + routeRequest + ResponseBuilder)
    }

    return 0;
}

