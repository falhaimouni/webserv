#include "../../includes/handlers/DirectoryHandler.hpp"
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <ctime>
#include <iomanip>
#include <vector>
#include <utility>

typedef struct stat StatInfo;

static std::string htmlEscape(const std::string &str)
{
    std::string escaped;
    escaped.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i)
    {
        switch (str[i])
        {
            case '&':  escaped += "&amp;";  break;
            case '"':  escaped += "&quot;"; break;
            case '\'': escaped += "&#39;";  break;
            case '<':  escaped += "&lt;";   break;
            case '>':  escaped += "&gt;";   break;
            default:   escaped += str[i];    break;
        }
    }
    return escaped;
}

static std::string formatFileSize(long size)
{
    std::ostringstream oss;
    if (size < 1024)
        oss << size << " B";
    else if (size < 1024 * 1024)
        oss << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
    else
        oss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
    return oss.str();
}

static std::string formatDate(time_t mtime)
{
    struct tm *timeinfo = localtime(&mtime);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", timeinfo);
    return std::string(buffer);
}

Response DirectoryHandler::handle(const std::string &path, const std::string &uri)
{
    Response res;
    std::ostringstream body;

    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        res.status = 500;
        res.reason = "Internal Server Error";
        res.body = "<h1>500 Internal Server Error</h1>";
        return res;
    }

    // Build parent URI
    std::string parentUri = uri;
    if (parentUri != "/")
    {
        size_t lastSlash = parentUri.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0)
            parentUri = parentUri.substr(0, lastSlash);
        else
            parentUri = "/";
    }

    body << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <title>Index of " << htmlEscape(uri) << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }\n"
         << "        .container { max-width: 900px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }\n"
         << "        h1 { color: #333; border-bottom: 2px solid #ddd; padding-bottom: 10px; margin-bottom: 20px; }\n"
         << "        table { width: 100%; border-collapse: collapse; }\n"
         << "        th { background-color: #f8f9fa; text-align: left; padding: 12px; border-bottom: 2px solid #ddd; color: #555; font-weight: 600; }\n"
         << "        td { padding: 10px 12px; border-bottom: 1px solid #eee; }\n"
         << "        tr:hover { background-color: #f8f9fa; }\n"
         << "        a { color: #0066cc; text-decoration: none; }\n"
         << "        a:hover { text-decoration: underline; }\n"
         << "        .icon { margin-right: 8px; }\n"
         << "        .size { text-align: right; color: #666; }\n"
         << "        .date { color: #666; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"container\">\n"
         << "        <h1>Index of " << htmlEscape(uri) << "</h1>\n"
         << "        <table>\n"
         << "            <thead>\n"
         << "                <tr>\n"
         << "                    <th>Name</th>\n"
         << "                    <th class=\"size\">Size</th>\n"
         << "                    <th>Date Modified</th>\n"
         << "                </tr>\n"
         << "            </thead>\n"
         << "            <tbody>\n";

    // Add parent directory link
    if (uri != "/")
    {
        struct stat parentSt;
        std::string parentPath = path + "/..";
        if (stat(parentPath.c_str(), &parentSt) == 0)
        {
            body << "                <tr>\n"
                 << "                    <td><a href=\"" << parentUri << "\"><span class=\"icon\">📁</span>..</a></td>\n"
                 << "                    <td class=\"size\">-</td>\n"
                 << "                    <td class=\"date\">" << formatDate(parentSt.st_mtime) << "</td>\n"
                 << "                </tr>\n";
        }
    }

    // Read directory entries
    struct dirent *entry;
    std::vector<std::pair<std::string, StatInfo> > entries;
    
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string fullPath = path;
        if (path[path.size() - 1] != '/')
            fullPath += "/";
        fullPath += name;

        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0)
        {
            entries.push_back(std::make_pair(name, st));
        }
    }
    closedir(dir);

    // Sort entries: directories first, then files, both alphabetically
    for (size_t i = 0; i < entries.size(); ++i)
    {
        for (size_t j = i + 1; j < entries.size(); ++j)
        {
            bool iIsDir = S_ISDIR(entries[i].second.st_mode);
            bool jIsDir = S_ISDIR(entries[j].second.st_mode);
            
            if ((!iIsDir && jIsDir) || 
                (iIsDir == jIsDir && entries[i].first > entries[j].first))
            {
                std::swap(entries[i], entries[j]);
            }
        }
    }

    // Output entries
    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::string name = entries[i].first;
        struct stat &st = entries[i].second;
        bool isDir = S_ISDIR(st.st_mode);
        
        std::string linkUri = uri;
        if (linkUri[linkUri.size() - 1] != '/')
            linkUri += "/";
        linkUri += name;
        if (isDir)
            linkUri += "/";

        body << "                <tr>\n"
             << "                    <td><a href=\"" << htmlEscape(linkUri) << "\">"
             << "<span class=\"icon\">" << (isDir ? "📁" : "📄") << "</span>"
             << htmlEscape(name) << (isDir ? "/" : "") << "</a></td>\n"
             << "                    <td class=\"size\">" << (isDir ? "-" : formatFileSize(st.st_size)) << "</td>\n"
             << "                    <td class=\"date\">" << formatDate(st.st_mtime) << "</td>\n"
             << "                </tr>\n";
    }

    body << "            </tbody>\n"
         << "        </table>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>\n";

    res.status = 200;
    res.reason = "OK";
    res.body = body.str();
    res.headers["Content-Type"] = "text/html";
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();

    return res;
}
