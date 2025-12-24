#include "handlers.hpp"
#include "response.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

// forward declaration from autoindex.cpp
std::string generate_autoindex(const std::string& dirPath, const std::string& requestPath);

static bool is_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static bool file_exists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

static std::string read_file(const std::string& path) {
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs) return std::string();
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static std::string guess_content_type(const std::string& path) {
    // very small mapping
    size_t pos = path.rfind('.');
    if (pos == std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(pos + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "txt") return "text/plain";
    return "application/octet-stream";
}

Response Handlers::handleGET(const HttpRequest& req, const LocationConfig& loc) {
    // root + request.path -> physical path
    // handle leading/trailing slashes
    std::string rel = req.path;
    if (rel.size() > 0 && rel[0] == '/') rel = rel.substr(1);
    std::string physical = loc.root;
    if (physical.size() == 0) physical = "./www";
    if (!physical.empty() && physical[physical.length() - 1] != '/')
        physical += "/";
    physical += rel;

    // If path ends with slash -> directory
    if (physical.size() > 0 && physical[physical.size() - 1] == '/') {
        // directory: try index
        std::string indexPath = physical + loc.index;
        if (!loc.index.empty() && file_exists(indexPath)) {
            std::string body = read_file(indexPath);
            Response r = make_200_ok(body, guess_content_type(indexPath));
            return r;
        } else if (loc.autoindex) {
            std::string html = generate_autoindex(physical, req.path);
            if (html.empty()) return make_error_response(403, "Autoindex not allowed or directory unreadable.");
            Response r = make_200_ok(html, "text/html");
            return r;
        } else {
            return make_error_response(403, "Directory listing denied.");
        }
    }

    // if physical is directory by stat (no trailing slash), treat as directory
    if (is_directory(physical)) {
        // redirect maybe or treat same as above
        std::string indexPath = physical;
        if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/')
            indexPath += "/";
        indexPath += loc.index;
        if (!loc.index.empty() && file_exists(indexPath)) {
            std::string body = read_file(indexPath);
            Response r = make_200_ok(body, guess_content_type(indexPath));
            return r;
        } else if (loc.autoindex) {
            std::string html = generate_autoindex(physical, req.path);
            if (html.empty()) return make_error_response(403, "Autoindex unreadable.");
            Response r = make_200_ok(html, "text/html");
            return r;
        } else {
            return make_error_response(403, "Directory listing denied.");
        }
    }

    // file case
    if (!file_exists(physical)) {
        return make_error_response(404, "File not found.");
    }

    std::string body = read_file(physical);
    if (body.empty() && (body.size() == 0)) {
        // Even empty file is ok; but if read failed -> 500
        // We'll check if read actually returned empty because file is empty or read failed:
        // Try to open to check
        std::ifstream ifs(physical.c_str());
        if (!ifs) {
            return make_error_response(500, "Failed to read file.");
        }
    }
    Response r = make_200_ok(body, guess_content_type(physical));
    return r;
}

Response Handlers::handlePOST(const HttpRequest& req, const LocationConfig& loc)
{
    if (loc.upload_dir.empty())
        return make_error_response(403, "Uploads not allowed.");

    struct stat st;
    if (stat(loc.upload_dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
        return make_error_response(404, "Upload directory not found.");

    if (access(loc.upload_dir.c_str(), W_OK) != 0)
        return make_error_response(403, "Upload directory not writable.");

    if (req.body.size() > loc.client_max_body_size)
        return make_error_response(413, "Payload Too Large.");

    std::string filename = loc.upload_dir + "/upload.data";

    int fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0)
        return make_error_response(500, strerror(errno));

    size_t total = 0;
    while (total < req.body.size())
    {
        ssize_t written = write(fd,
            req.body.c_str() + total,
            req.body.size() - total);

        if (written < 0)
        {
            close(fd);
            return make_error_response(500, strerror(errno));
        }
        total += written;
    }

    close(fd);

    Response r;
    r.status = 201;
    r.status_message = "Created";
    r.body = "<html><body><h1>Upload OK</h1></body></html>";
    r.headers["Content-Type"] = "text/html";
    return r;
}