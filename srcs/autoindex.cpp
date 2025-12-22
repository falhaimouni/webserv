#include <string>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

std::string generate_autoindex(const std::string& dirPath, const std::string& requestPath) {
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) {
        return std::string();
    }
    std::vector<std::string> entries;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        std::string name = ent->d_name;
        if (name == ".") continue;
        entries.push_back(name);
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end());
    std::stringstream ss;
    ss << "<html><head><title>Index of " << requestPath << "</title></head>\n";
    ss << "<body><h1>Index of " << requestPath << "</h1><ul>\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        std::string e = entries[i];
        ss << "<li><a href=\"" << e << "\">" << e << "</a></li>\n";
    }
    ss << "</ul><hr><p>autoindex</p></body></html>\n";
    return ss.str();
}
