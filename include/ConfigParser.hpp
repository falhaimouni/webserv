#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ServerConfig.hpp"
#include "Logger.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <cctype>  
#include <cstdlib>

class ConfigParser
{
private:
    std::vector<ServerConfig> _servers;

    std::vector<std::string> tokenizer(const std::string &path);

public:
    ConfigParser();
    ~ConfigParser();

    bool parse(const std::string &path);
    const std::vector<ServerConfig> &getServers() const;
};

#endif
