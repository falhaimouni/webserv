#include "ConfigParser.hpp"

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
        bool hasListen = false;

        while (i < toks.size() && toks[i] != "}")
        {
            if (toks[i] == "listen")
            {
                i++;
                if (i >= toks.size())
                {
                    Logger::error("Expected port number after listen");
                    return false;
                }

                srv.port = std::atoi(toks[i].c_str());
                hasListen = true;
                i++;

                if (i >= toks.size() || toks[i] != ";")
                {
                    Logger::error("Expected ';' after listen <port>");
                    return false;
                }
                i++;
            }
            else
            {
                while (i < toks.size())
                {
                    if (toks[i] == ";" || toks[i] == "{" || toks[i] == "}")
                        break;
                    i++;
                }

                if (i < toks.size() && toks[i] == ";")
                    i++;

                if (i < toks.size() && toks[i] == "{")
                {
                    int depth = 0;
                    while (i < toks.size())
                    {
                        if (toks[i] == "{") depth++;
                        else if (toks[i] == "}") depth--;
                        i++;
                        if (depth == 0)
                            break;
                    }
                }
            }
        }

        if (i >= toks.size() || toks[i] != "}")
        {
            Logger::error("Missing '}' to close server block");
            return false;
        }
        i++;

        if (!hasListen)
        {
            Logger::error("server block missing 'listen'");
            return false;
        }

        _servers.push_back(srv);
    }

    return !_servers.empty();
}
