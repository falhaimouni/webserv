#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../shared/Response.hpp"
#include "../shared/Request.hpp"
#include "../shared/LocationConfig.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sys/types.h>

class CGIHandler
{
private:
    static std::string getFileExt(const std::string &scriptPath, const LocationConfig &config);
    static std::vector<std::string> buildEnvVars(const Request &req, const std::string &scriptPath, const LocationConfig &config);
    static char** envToArray(const std::vector<std::string> &envVars);
    static void freeEnvArray(char **envArray);
    static std::string readFromPipe(int fd, int timeoutSeconds);
    static bool isExecutable(const std::string &path, bool useInterpreter);
    
public:
    struct CgiProcess
    {
        int inFd;
        int outFd;
        pid_t pid;
        std::string input;
        size_t inputOffset;
        std::string output;
        std::time_t startTime;
        int timeoutSec;
        std::string scriptPath;

        CgiProcess()
            : inFd(-1), outFd(-1), pid(-1), inputOffset(0), startTime(0), timeoutSec(30)
        {}
    };

    static bool startCgiProcess(const Request &req,
                                const LocationConfig &config,
                                const std::string &scriptPath,
                                CgiProcess &proc,
                                Response &errorRes);
    static Response buildCgiResponse(const std::string &cgiOutput, const std::string &scriptPath);
};


#endif