#include <iostream>
#include <fstream>
#include <string>
#include "srcs/http/HttpParser.hpp"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <http_request_file>\n";
        return 1;
    }

    // 🔹 Read the file as binary to preserve CRLF exactly
    std::ifstream file(argv[1], std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }

    std::string buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    HttpParser parser;

    // 🔹 Feed the buffer to the parser
    parser.feed(buffer);

    if (parser.hasError())
    {
        std::cout << "Parsing failed: ERROR\n";
        return 1;
    }

    if (!parser.isComplete())
    {
        std::cout << "Parsing incomplete: waiting for more data\n";
        return 1;
    }

    // 🔹 Print the parsed request
    const Request& req = parser.getRequest();

    std::cout << "==== PARSED REQUEST ====\n";
    std::cout << "Method  : " << req.method << "\n";
    std::cout << "Path    : " << req.path << "\n";
    std::cout << "Version : " << req.version << "\n";

    std::cout << "\nHeaders:\n";
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
         it != req.headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << "\n";
    }

    std::cout << "\nBody:\n";
    if (req.body.empty())
        std::cout << "(empty)\n";
    else
        std::cout << req.body << "\n";

    return 0;
}
