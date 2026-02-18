#include "server/ConfigParser.hpp"

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

const std::vector<ServerConfig> &ConfigParser::getServers() const
{
    return _servers;
}

static bool specialSymbols(char c)
{
    return (c == '{' || c == '}' || c == ';');
}

static bool isNumber(const std::string &s)
{
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    return true;
}

static bool isValidMethod(const std::string &m)
{
    return (m == "GET" || m == "POST" || m == "DELETE");
}

static bool parseBoolOnOff(const std::string &s, bool &out)
{
    if (s == "on")
    {
        out = true;
        return true;
    }
    if (s == "off")
    {
        out = false;
        return true;
    }
    return false;
}

static bool parseSizeToken(const std::string &s, size_t &out)
{
    if (s.empty())
        return false;

    size_t i = 0;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        ++i;

    if (i == 0)
        return false;

    std::string numberPart = s.substr(0, i);
    unsigned long long value = 0;
    for (size_t k = 0; k < numberPart.size(); ++k)
        value = value * 10 + static_cast<unsigned long long>(numberPart[k] - '0');

    unsigned long long multiplier = 1;
    if (i < s.size())
    {
        if (s.size() != i + 1)
            return false;
        char suffix = s[i];
        if (suffix == 'K' || suffix == 'k')
            multiplier = 1024ULL;
        else if (suffix == 'M' || suffix == 'm')
            multiplier = 1024ULL * 1024ULL;
        else if (suffix == 'G' || suffix == 'g')
            multiplier = 1024ULL * 1024ULL * 1024ULL;
        else
            return false;
    }

    unsigned long long total = value * multiplier;
    if (total == 0)
        return false;
    out = static_cast<size_t>(total);
    return true;
}

static bool isValidIpv4(const std::string &s)
{
    if (s == "localhost")
        return true;
    size_t start = 0;
    int parts = 0;
    while (start < s.size())
    {
        size_t dot = s.find('.', start);
        std::string part = (dot == std::string::npos) ? s.substr(start) : s.substr(start, dot - start);
        if (part.empty() || part.size() > 3 || !isNumber(part))
            return false;
        int val = std::atoi(part.c_str());
        if (val < 0 || val > 255)
            return false;
        parts++;
        if (dot == std::string::npos)
            break;
        start = dot + 1;
    }
    return parts == 4;
}

static bool expectToken(const std::vector<std::string> &toks, size_t &i, const std::string &expected)
{
    if (i >= toks.size() || toks[i] != expected)
        return false;
    i++;
    return true;
}

std::vector<std::string> ConfigParser::tokenizer(const std::string &path)
{
    std::vector<std::string> toks;
    std::ifstream in(path.c_str());
    if (!in.is_open())
        return toks;

    std::string content;
    std::string line;

    while(std::getline(in, line))
        content += line + "\n";
    
    std::string cur;
    for (int i = 0; i < (int)content.size(); i++)
    {
        char c = content[i];

        if (c == '#') // Skip comments: everything after # until end of line
        {
            while (i < (int)content.size() && content[i] != '\n')
                i++;
            continue;
        }

        if (std::isspace((unsigned char)c))  // Whitespace ends the current token
        {
            if (!cur.empty())
            {
                toks.push_back(cur);
                cur.clear();
            }
            continue;
        }

        if (c == '\r') // Extra safety for Windows files (CRLF)
            continue;
        
        if (specialSymbols(c)) // Special symbols are their own tokens
        {
            if (!cur.empty())
            {
                toks.push_back(cur);
                cur.clear();
            }
            toks.push_back(std::string(1, c));
            continue;
        }

        cur += c;
    }

    if (!cur.empty())
        toks.push_back(cur);

    return toks;
}

bool ConfigParser::parse(const std::string &path)
{
    _servers.clear();

    std::vector<std::string> toks = tokenizer(path);
    if (toks.empty())
    {
        Logger::error("Config file empty or not found: " + path);
        return false;
    }

    size_t i = 0;

    while (i < toks.size())
    {
        if (toks[i] != "server")
        {
            i++;        
            continue;
        }

        i++;
        if (i >= toks.size() || toks[i] != "{")
        {
            Logger::error("Expected '{' after server");
            return false;
        }
        i++;

        ServerConfig srv;
        std::vector<std::pair<std::string, int> > listenAddrs; // Store all listen directives (host, port)

        while (i < toks.size() && toks[i] != "}")
        {
            if (toks[i] == "listen")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected port or IP:port after listen");
                    return false;
                }
                
                std::string host = "0.0.0.0";
                int port = 0;
                
                // Check if it's IP:port format
                size_t colonPos = toks[i].find(':');
                if (colonPos != std::string::npos)
                {
                    // Format: IP:port
                    std::string ipPart = toks[i].substr(0, colonPos);
                    std::string portPart = toks[i].substr(colonPos + 1);
                    
                    if (!isValidIpv4(ipPart))
                    {
                        Logger::error("Invalid IP address in listen: " + ipPart);
                        return false;
                    }
                    if (!isNumber(portPart))
                    {
                        Logger::error("Invalid port in listen: " + portPart);
                        return false;
                    }
                    
                    host = ipPart;
                    port = std::atoi(portPart.c_str());
                }
                else
                {
                    // Format: port only
                    if (!isNumber(toks[i]))
                    {
                        Logger::error("Invalid listen port: " + toks[i]);
                        return false;
                    }
                    port = std::atoi(toks[i].c_str());
                }
                
                if (port <= 0 || port > 65535)
                {
                    Logger::error("listen port out of range: " + toks[i]);
                    return false;
                }
                
                listenAddrs.push_back(std::make_pair(host, port));
                i++;

                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after listen directive");
                    return false;
                }
                i++;
            }
            else if (toks[i] == "host")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected host after host");
                    return false;
                }
                if (!isValidIpv4(toks[i]))
                {
                    Logger::error("Invalid host: " + toks[i]);
                    return false;
                }
                srv.host = toks[i];
                i++;
                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after host");
                    return false;
                }
                i++;
            }
            else if (toks[i] == "server_name")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected server_name value");
                    return false;
                }
                srv.server_name = toks[i];
                i++;
                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after server_name");
                    return false;
                }
                i++;
            }
            else if (toks[i] == "root")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected root path");
                    return false;
                }
                srv.root = toks[i];
                i++;
                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after root");
                    return false;
                }
                i++;
            }
            else if (toks[i] == "index")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected index value");
                    return false;
                }
                srv.index = toks[i];
                i++;
                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after index");
                    return false;
                }
                i++;
            }
            else if (toks[i] == "client_max_body_size")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected client_max_body_size value");
                    return false;
                }
                size_t sizeVal = 0;
                if (!parseSizeToken(toks[i], sizeVal))
                {
                    Logger::error("Invalid client_max_body_size: " + toks[i]);
                    return false;
                }
                srv.client_max_body_size = sizeVal;
                i++;
                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after client_max_body_size");
                    return false;
                }
                i++;
            }
            else if (toks[i] == "error_page")
            {
                i++;
                if (i + 1 >= toks.size())
                {
                    Logger::error("Expected error_page <code> <path>");
                    return false;
                }
                if (!isNumber(toks[i]))
                {
                    Logger::error("Invalid error code: " + toks[i]);
                    return false;
                }
                int code = std::atoi(toks[i].c_str());
                if (code < 400 || code > 599)
                {
                    Logger::error("Error code out of range: " + toks[i]);
                    return false;
                }
                i++;
                std::string pathVal = toks[i];
                i++;
                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after error_page");
                    return false;
                }
                i++;
                srv.error_pages[code] = pathVal;
            }
            else if (toks[i] == "location")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected path after location");
                    return false;
                }
                std::string locPath = toks[i];
                if (locPath.empty() || locPath[0] != '/')
                {
                    Logger::error("Invalid location path: " + locPath);
                    return false;
                }
                i++;
                if (!expectToken(toks, i, "{"))
                {
                    Logger::error("Expected '{' after location path");
                    return false;
                }

                LocationConfig loc;
                loc.path = locPath;
                loc.root = srv.root;
                loc.index = srv.index;
                loc.error_pages = srv.error_pages;
                loc.max_body_size = srv.client_max_body_size;

                while (i < toks.size() && toks[i] != "}")
                {
                    if (toks[i] == "methods" || toks[i] == "allowed_methods")
                    {
                        i++;
                        loc.methods.clear();
                        if (i >= toks.size())
                        {
                            Logger::error("Expected methods list");
                            return false;
                        }
                        while (i < toks.size() && toks[i] != ";")
                        {
                            if (!isValidMethod(toks[i]))
                            {
                                Logger::error("Invalid method: " + toks[i]);
                                return false;
                            }
                            loc.methods.insert(toks[i]);
                            i++;
                        }
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after methods");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "autoindex")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected on/off after autoindex");
                            return false;
                        }
                        bool val = false;
                        if (!parseBoolOnOff(toks[i], val))
                        {
                            Logger::error("Invalid autoindex value: " + toks[i]);
                            return false;
                        }
                        loc.autoindex = val;
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after autoindex");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "root")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected root path in location");
                            return false;
                        }
                        loc.root = toks[i];
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after location root");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "index")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected index in location");
                            return false;
                        }
                        loc.index = toks[i];
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after location index");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "upload_enable")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected on/off after upload_enable");
                            return false;
                        }
                        bool val = false;
                        if (!parseBoolOnOff(toks[i], val))
                        {
                            Logger::error("Invalid upload_enable value: " + toks[i]);
                            return false;
                        }
                        loc.upload_enable = val;
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after upload_enable");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "upload_store")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected upload_store path");
                            return false;
                        }
                        loc.upload_store = toks[i];
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after upload_store");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "cgi_extension")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected cgi_extension value");
                            return false;
                        }
                        loc.is_cgi = true;
                        
                        // Parse: cgi_extension .ext /path/to/interpreter;
                        // First token should be the extension
                        if (toks[i].empty() || toks[i][0] != '.')
                        {
                            Logger::error("Invalid cgi_extension: " + toks[i]);
                            return false;
                        }
                        std::string ext = toks[i];
                        loc.cgi_ext.insert(ext);
                        i++;
                        
                        // Next token might be the interpreter path, or ';'
                        if (i < toks.size() && toks[i] != ";")
                        {
                            // We have an interpreter path
                            loc.cgi_interpreters[ext] = toks[i];
                            i++;
                        }
                        
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after cgi_extension");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "cgi_path")
                    {
                        i++;
                        if (i >= toks.size())
                        {
                            Logger::error("Expected cgi_path value");
                            return false;
                        }
                        loc.cgi_path = toks[i];
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after cgi_path");
                            return false;
                        }
                        i++;
                    }
                    else if (toks[i] == "error_page")
                    {
                        i++;
                        if (i + 1 >= toks.size())
                        {
                            Logger::error("Expected error_page <code> <path> in location");
                            return false;
                        }
                        if (!isNumber(toks[i]))
                        {
                            Logger::error("Invalid error code: " + toks[i]);
                            return false;
                        }
                        int code = std::atoi(toks[i].c_str());
                        if (code < 400 || code > 599)
                        {
                            Logger::error("Error code out of range: " + toks[i]);
                            return false;
                        }
                        i++;
                        std::string pathVal = toks[i];
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after location error_page");
                            return false;
                        }
                        i++;
                        loc.error_pages[code] = pathVal;
                    }
                    else if (toks[i] == "return")
                    {
                        i++;
                        if (i + 1 >= toks.size())
                        {
                            Logger::error("Expected return <code> <url>");
                            return false;
                        }
                        if (!isNumber(toks[i]))
                        {
                            Logger::error("Invalid redirect code: " + toks[i]);
                            return false;
                        }
                        int code = std::atoi(toks[i].c_str());
                        if (code < 300 || code > 399)
                        {
                            Logger::error("Redirect code out of range (300-399): " + toks[i]);
                            return false;
                        }
                        loc.redirect_code = code;
                        i++;
                        loc.redirect_to = toks[i];
                        i++;
                        if (i >= toks.size() || toks[i] != ";")
                        {
                            Logger::error("Expected ';' after return directive");
                            return false;
                        }
                        i++;
                    }
                    else
                    {
                        Logger::error("Unknown directive in location: " + toks[i]);
                        return false;
                    }
                }

                if (!expectToken(toks, i, "}"))
                {
                    Logger::error("Missing '}' to close location block");
                    return false;
                }

                srv.locations.push_back(loc);
            }
            else
            {
                Logger::error("Unknown directive in server: " + toks[i]);
                return false;
            }
        }

        if (i >= toks.size() || toks[i] != "}")
        {
            Logger::error("Missing '}' to close server block");
            return false;
        }
        i++;

        if (listenAddrs.empty())
        {
            Logger::error("server block missing 'listen'");
            return false;
        }

        // Create a separate ServerConfig for each listen directive
        for (size_t j = 0; j < listenAddrs.size(); ++j)
        {
            ServerConfig serverCopy = srv;
            serverCopy.host = listenAddrs[j].first;
            serverCopy.port = listenAddrs[j].second;
            _servers.push_back(serverCopy);
        }
    }

    return !_servers.empty();
}
