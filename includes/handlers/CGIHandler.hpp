#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../shared/Response.hpp"
#include "../shared/Request.hpp"
#include "../shared/LocationConfig.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>

class CGIHandler
{
private:
    static std::string getInterpreter(const std::string &scriptPath, const LocationConfig &config);
    static std::vector<std::string> buildEnvVars(const Request &req, const std::string &scriptPath, const LocationConfig &config);
    static char** envToArray(const std::vector<std::string> &envVars);
    static void freeEnvArray(char **envArray);
    static std::string readFromPipe(int fd, int timeoutSeconds);
    static bool isExecutable(const std::string &path);
    
public:
    static Response handleCGIReq(const std::string &scriptPath, const std::string &input);
    static Response handleCGIReq(const Request &req, const LocationConfig &config, const std::string &scriptPath);
};


#endif