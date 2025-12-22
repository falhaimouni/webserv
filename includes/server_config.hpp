#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::vector<std::string> allowed_methods;
    std::string root;
    bool autoindex;
    std::string index;
    std::string upload_dir;
    std::map<std::string, std::string> cgi_extensions;
    size_t client_max_body_size;
    int redirect_code;
    std::string redirect_url;

    LocationConfig()
        : autoindex(false), client_max_body_size(1000000),
          redirect_code(0) {}
};

#endif
