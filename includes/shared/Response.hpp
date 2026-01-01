#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <iostream>
#include <sstream>


struct Response {
    int status;
    std::string reason;

    std::map<std::string, std::string> headers;
    std::string body;
};

#endif

