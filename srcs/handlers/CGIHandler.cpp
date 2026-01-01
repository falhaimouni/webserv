#include "../../includes/handlers/CGIHandler.hpp"
#include <fstream>

Response CGIHandler::handleCGIReq(const std::string &scriptPath, const std::string &input)
{
    Response res;
    (void)input;
    // Simple simulation of CGI execution
    // In real scenario, you would fork a process, set up environment variables, and execute the script
    std::ifstream scriptFile(scriptPath.c_str());
    if (!scriptFile.is_open())
    {
        res.status = 500;
        res.reason = "Internal Server Error";
        res.headers["Content-Type"] = "text/html";
        res.body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    }
    else
    {
        std::string line;
        std::string output;
        while (std::getline(scriptFile, line))
        {
            output += line + "\n";
        }
        scriptFile.close();
        
        res.status = 200;
        res.reason = "OK";
        res.headers["Content-Type"] = "text/html";
        res.body = output; // In real CGI, this would be the output of the script execution
    }
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}