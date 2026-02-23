#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include "../shared/LocationConfig.hpp"

struct ServerConfig
{
    int port;
    std::string host;
    std::string server_name;
    std::string root;
    std::string index;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;

    ServerConfig()
        : port(8080),
          host("0.0.0.0"),
          server_name(""),
          root("./www"),
          index("index.html"),
          client_max_body_size(1000000)
    {}
};

#endif

