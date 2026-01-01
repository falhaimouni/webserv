#include "../includes/handlers/StaticHandler.hpp"
#include "../includes/handlers/ResponseBuilder.hpp"
#include "../includes/handlers/UploadHandler.hpp"
#include "../includes/handlers/CGIHandler.hpp"
#include "../includes/handlers/ErrorHandler.hpp"


std::string parse_request(const std::string &request)
{
    size_t pos = request.find(" ");
    if (pos == std::string::npos)
        return "";
    size_t pos2 = request.find(" ", pos + 1);
    if (pos2 == std::string::npos)
        return "";
    return request.substr(pos + 1, pos2 - pos - 1);
}

// std::string get_path(std::string request)
// {
    
// }

void call_handler(const std::string &path)
{
    if (path == "/static")
    {
        Response res = StaticHandler::handleStaticReq("index.html");
        std::cout << ResponseBuilder::formatResponse(res) << "\n";
    }
    else if (path == "/upload")
    {
        Response res = UploadHandler::handleUploadReq("upload_test.txt", "Hello Upload!");
        std::cout << ResponseBuilder::formatResponse(res) << "\n";
    }
    else if (path == "/cgi-bin")
    {
        Response res = CGIHandler::handleCGIReq("test_script.txt", "input_data");
        std::cout << ResponseBuilder::formatResponse(res) << "\n";
    }
    else
    {
        Response res = ErrorHandler::handleError(404, "Not Found");
        std::cout << ResponseBuilder::formatResponse(res) << "\n";
    }
}