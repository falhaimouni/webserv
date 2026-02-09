#include "HttpHandler.hpp"
#include "Logger.hpp"

static std::string getFirstLine(const std::string &req)
{
    size_t pos = req.find("\r\n");
    if (pos == std::string::npos)
        return req;
    return req.substr(0, pos);
}

static void split(const std::string &line, std::string &a, std::string &b, std::string &c)
{
    a = ""; b = ""; c = "";

    size_t p1 = line.find(' ');
    if (p1 == std::string::npos) return;
    size_t p2 = line.find(' ', p1 + 1);
    if (p2 == std::string::npos) return;

    a = line.substr(0, p1);
    b = line.substr(p1 + 1, p2 - (p1 + 1));
    c = line.substr(p2 + 1);
}

std::string HttpHandler::buildResponse(int code, const std::string &body)
{
    std::string status = "500 Internal Server Error";
    if (code == 200) status = "200 OK";
    else if (code == 404) status = "404 Not Found";
    else if (code == 405) status = "405 Method Not Allowed";

    std::string res;
    res += "HTTP/1.1 " + status + "\r\n";
    res += "Content-Type: text/plain\r\n";
    res += "Content-Length: " + Logger::toStringInt((int)body.size()) + "\r\n";
    res += "Connection: close\r\n";
    res += "\r\n";
    res += body;
    return res;
}

void HttpHandler::handle(Client &client)
{
    std::string line = getFirstLine(client.readBuffer);

    std::string method, path, version;
    split(line, method, path, version);

    Logger::info("Request-Line: [" + line + "]");

    // Very basic validation
    if (method == "" || path == "" || version == "")
    {
        client.writeBuffer = buildResponse(404, "Bad Request Line\n");
        client.wantClose = true;
        return;
    }

    if (method != "GET")
    {
        client.writeBuffer = buildResponse(405, "Only GET supported for now\n");
        client.wantClose = true;
        return;
    }

    if (path == "/")
        client.writeBuffer = buildResponse(200, "Hello from webserv!\n");
    else
        client.writeBuffer = buildResponse(404, "Not Found\n");

    client.wantClose = true;
}
