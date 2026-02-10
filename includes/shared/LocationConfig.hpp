#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <set>

struct LocationConfig {
    std::string path;              // "/upload"
    std::string root;              // "./www"
    std::set<std::string> methods; // GET POST DELETE

    // Directory behavior
    std::string index;             // "index.html"
    bool autoindex;                // directory listing

    // Upload
    std::string upload_store;      // "./uploads"

    // CGI
    bool is_cgi;
    std::set<std::string> cgi_ext; // ".py", ".php"

    std::string redirect_to; // if non-empty, we redirect
};

#endif

