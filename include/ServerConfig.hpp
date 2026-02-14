#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include "../includes/shared/LocationConfig.hpp"

struct ServerConfig
{
    int port;
    std::string root;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;

    ServerConfig() : port(8080), root("./www"), client_max_body_size(1000000) {}
};

#endif

