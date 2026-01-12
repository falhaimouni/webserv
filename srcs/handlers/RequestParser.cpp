#include "../../includes/handlers/RequestParser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

static std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

Request RequestParser::parse(const std::string &raw)
{
    Request req;

    // Parse request line
    size_t pos1 = raw.find(' ');
    if (pos1 == std::string::npos)
        return req;
    req.method = raw.substr(0, pos1);

    size_t pos2 = raw.find(' ', pos1 + 1);
    if (pos2 == std::string::npos)
        return req;
    req.path = raw.substr(pos1 + 1, pos2 - pos1 - 1);
    
    // Extract version
    size_t lineEnd = raw.find("\r\n", pos2 + 1);
    if (lineEnd == std::string::npos)
        lineEnd = raw.find("\n", pos2 + 1);
    if (lineEnd != std::string::npos)
    {
        req.version = raw.substr(pos2 + 1, lineEnd - pos2 - 1);
        req.version = trim(req.version);
    }
    
    // Clean path
    if (!req.path.empty() && req.path[req.path.size() - 1] == '\r')
        req.path.resize(req.path.size() - 1);
    
    // Find header section
    size_t headerEnd = raw.find("\r\n\r\n");
    size_t headerStart = lineEnd != std::string::npos ? lineEnd + 2 : 0;
    
    if (headerEnd == std::string::npos)
    {
        headerEnd = raw.find("\n\n");
        if (headerEnd != std::string::npos)
            headerStart = lineEnd != std::string::npos ? lineEnd + 1 : 0;
    }
    
    // Parse headers
    if (headerEnd != std::string::npos && headerStart < headerEnd)
    {
        std::string headerSection = raw.substr(headerStart, headerEnd - headerStart);
        std::istringstream headerStream(headerSection);
        std::string headerLine;
        
        while (std::getline(headerStream, headerLine))
        {
            // Remove CR if present
            if (!headerLine.empty() && headerLine[headerLine.size() - 1] == '\r')
                headerLine.resize(headerLine.size() - 1);
            
            size_t colonPos = headerLine.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = headerLine.substr(0, colonPos);
                std::string value = headerLine.substr(colonPos + 1);
                
                // Trim key and value
                key = trim(key);
                value = trim(value);
                
                // Convert key to lowercase for case-insensitive matching
                std::string keyLower = key;
                std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
                
                req.headers[keyLower] = value;
            }
        }
        
        // Extract body
        size_t bodyStart = headerEnd + (raw.find("\r\n\r\n") != std::string::npos ? 4 : 2);
        if (bodyStart < raw.size())
        {
            req.body = raw.substr(bodyStart);
            
            // Trim trailing newlines from body
            while (!req.body.empty() && 
                  (req.body[req.body.size() - 1] == '\r' || req.body[req.body.size() - 1] == '\n'))
            {
                req.body.resize(req.body.size() - 1);
            }
        }
    }
    
    return req;
}
