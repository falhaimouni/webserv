#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <set>
#include <map>
#include <cstddef>

struct LocationConfig {
    std::string path;              // "/upload"
    std::string root;              // "./www"
    std::set<std::string> methods; // GET POST DELETE

    // Directory behavior
    std::string index;             // "index.html"
    bool autoindex;                // directory listing

    // Upload
    bool upload_enable;
    std::string upload_store;      // "./uploads"

    // Error pages
    std::map<int, std::string> error_pages;

    // CGI
    bool is_cgi;
    std::set<std::string> cgi_ext; // ".py", ".php"
    std::map<std::string, std::string> cgi_interpreters; // extension -> interpreter path
    std::string cgi_path;

    // Redirects
    std::string redirect_to; // if non-empty, we redirect
    int redirect_code;

    // Limits
    size_t max_body_size;

    LocationConfig()
        : path("/"),
          root("./www"),
          index("index.html"),
          autoindex(false),
          upload_enable(false),
          upload_store("./uploads"),
          is_cgi(false),
          cgi_path(""),
          redirect_code(0),
          max_body_size(1000000)
    {
        methods.insert("GET");
        methods.insert("POST");
        methods.insert("DELETE");
    }
};

#endif

