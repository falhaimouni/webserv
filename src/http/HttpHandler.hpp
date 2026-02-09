#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include "Client.hpp"

class HttpHandler
{
    private:
        static std::string buildResponse(int code, const std::string &body);
    public:
        static void handle(Client &client);
};

#endif