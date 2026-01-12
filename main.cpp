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


#include "includes/server/Server.hpp"
#include <iostream>

int main()
{
    try
    {
        Server server(8080);
        server.socketSetup();
        server.run();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}


