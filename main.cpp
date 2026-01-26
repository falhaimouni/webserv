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


