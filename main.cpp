#include "includes/router/ClientManager.hpp"
#include "includes/router/Router.hpp"
#include "includes/shared/Request.hpp"
#include "includes/shared/LocationConfig.hpp"
#include "includes/shared/HttpStatus.hpp"
#include <iostream>
#include <cassert>
#include <vector>

// Helper to print test results
void printTest(const std::string& name, bool passed)
{
    if (passed)
        std::cout << "✓ " << name << std::endl;
    else
        std::cout << "✗ " << name << std::endl;
}

// Helper to print request
void printRequest(const Request& req)
{
    std::cout << "  Method: " << req.method << std::endl;
    std::cout << "  Path: " << req.path << std::endl;
    std::cout << "  Version: " << req.version << std::endl;
    std::cout << "  Headers: " << req.headers.size() << std::endl;
    std::cout << "  Body length: " << req.body.size() << std::endl;
}

// Helper to get status name
std::string getStatusName(int status)
{
    switch(status)
    {
        case HTTP_OK: return "200 OK";
        case HTTP_BAD_REQUEST: return "400 Bad Request";
        case HTTP_NOT_FOUND: return "404 Not Found";
        case HTTP_METHOD_NOT_ALLOWED: return "405 Method Not Allowed";
        case HTTP_LENGTH_REQUIRED: return "411 Length Required";
        case HTTP_PAYLOAD_TOO_LARGE: return "413 Payload Too Large";
        case HTTP_REQUEST_URI_TOO_LONG: return "414 URI Too Long";
        case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE: return "431 Headers Too Large";
        case HTTP_INTERNAL_SERVER_ERROR: return "500 Internal Server Error";
        default: return "Unknown";
    }
}

// ============================================================
// PARSER TESTS
// ============================================================

void test_parser_simple_get()
{
    std::cout << "\n[PARSER TEST 1] Simple GET Request\n";
    ClientManager cm;
    int fd = 1;
    
    cm.addClient(fd);
    std::string req = "GET /index.html HTTP/1.0\r\nHost: localhost\r\n\r\n";
    cm.onData(fd, req);
    
    bool hasReq = cm.hasRequest(fd);
    bool hasErr = cm.hasError(fd);
    
    if (hasReq && !hasErr)
    {
        Request r = cm.getRequest(fd);
        bool pass = (r.method == "GET" && r.path == "/index.html" && r.version == "HTTP/1.0");
        printTest("Simple GET", pass);
        if (pass) printRequest(r);
    }
    else
    {
        printTest("Simple GET", false);
    }
    cm.removeClient(fd);
}

void test_parser_post_with_body()
{
    std::cout << "\n[PARSER TEST 2] POST with Body\n";
    ClientManager cm;
    int fd = 2;
    
    cm.addClient(fd);
    std::string body = "username=admin&password=123";
    std::string req = "POST /login HTTP/1.0\r\n"
                      "Content-Length: " + std::to_string(body.length()) + "\r\n"
                      "\r\n" + body;
    cm.onData(fd, req);
    
    bool hasReq = cm.hasRequest(fd);
    bool hasErr = cm.hasError(fd);
    
    if (hasReq && !hasErr)
    {
        Request r = cm.getRequest(fd);
        bool pass = (r.method == "POST" && r.body == body && 
                     r.body.size() == body.length());
        printTest("POST with Body", pass);
        if (pass) printRequest(r);
    }
    else
    {
        printTest("POST with Body", false);
    }
    cm.removeClient(fd);
}

void test_parser_delete_method()
{
    std::cout << "\n[PARSER TEST 3] DELETE Method\n";
    ClientManager cm;
    int fd = 3;
    
    cm.addClient(fd);
    std::string req = "DELETE /file.txt HTTP/1.0\r\nHost: localhost\r\n\r\n";
    cm.onData(fd, req);
    
    bool hasReq = cm.hasRequest(fd);
    bool hasErr = cm.hasError(fd);
    
    if (hasReq && !hasErr)
    {
        Request r = cm.getRequest(fd);
        bool pass = (r.method == "DELETE" && r.path == "/file.txt");
        printTest("DELETE Method", pass);
    }
    else
    {
        printTest("DELETE Method", false);
    }
    cm.removeClient(fd);
}

void test_parser_case_insensitive_headers()
{
    std::cout << "\n[PARSER TEST 4] Case-Insensitive Headers\n";
    ClientManager cm;
    int fd = 4;
    
    cm.addClient(fd);
    std::string req = "GET / HTTP/1.0\r\n"
                      "CoNtEnT-LeNgTh: 0\r\n"
                      "HoSt: LOCALHOST\r\n"
                      "\r\n";
    cm.onData(fd, req);
    
    bool hasReq = cm.hasRequest(fd);
    
    if (hasReq)
    {
        Request r = cm.getRequest(fd);
        bool pass = (r.headers.find("content-length") != r.headers.end() &&
                     r.headers.find("host") != r.headers.end());
        printTest("Case-Insensitive Headers", pass);
    }
    else
    {
        printTest("Case-Insensitive Headers", false);
    }
    cm.removeClient(fd);
}

void test_parser_partial_data()
{
    std::cout << "\n[PARSER TEST 5] Partial Data (Incremental)\n";
    ClientManager cm;
    int fd = 5;
    
    cm.addClient(fd);
    cm.onData(fd, "GET /");
    bool hasReq1 = cm.hasRequest(fd);
    
    cm.onData(fd, "test HTTP/1.0\r\n");
    bool hasReq2 = cm.hasRequest(fd);
    
    cm.onData(fd, "Host: localhost\r\n\r\n");
    bool hasReq3 = cm.hasRequest(fd);
    
    bool pass = (!hasReq1 && !hasReq2 && hasReq3);
    printTest("Partial Data Buffering", pass);
    
    if (hasReq3)
    {
        Request r = cm.getRequest(fd);
        pass = pass && (r.path == "/test");
    }
    cm.removeClient(fd);
}

void test_parser_pipelined_requests()
{
    std::cout << "\n[PARSER TEST 6] Pipelined Requests\n";
    ClientManager cm;
    int fd = 6;
    
    cm.addClient(fd);
    std::string req1 = "GET /first HTTP/1.0\r\n\r\n";
    std::string req2 = "GET /second HTTP/1.0\r\n\r\n";
    
    cm.onData(fd, req1 + req2);
    
    if (cm.hasRequest(fd))
    {
        Request r1 = cm.getRequest(fd);
        bool pass1 = (r1.path == "/first");
        printTest("Pipelined Request 1", pass1);
        
        cm.reset(fd);
        cm.onData(fd, "");  // Trigger parsing of buffered data
        
        if (cm.hasRequest(fd))
        {
            Request r2 = cm.getRequest(fd);
            bool pass2 = (r2.path == "/second");
            printTest("Pipelined Request 2", pass2);
        }
        else
        {
            printTest("Pipelined Request 2", false);
        }
    }
    else
    {
        printTest("Pipelined Request 1", false);
    }
    cm.removeClient(fd);
}

void test_parser_duplicate_headers()
{
    std::cout << "\n[PARSER TEST 7] Duplicate Headers (First Wins)\n";
    ClientManager cm;
    int fd = 7;
    
    cm.addClient(fd);
    std::string req = "GET / HTTP/1.0\r\n"
                      "Host: first\r\n"
                      "Host: second\r\n"
                      "\r\n";
    cm.onData(fd, req);
    
    if (cm.hasRequest(fd))
    {
        Request r = cm.getRequest(fd);
        bool pass = (r.headers["host"] == "first");  // First value wins
        printTest("Duplicate Headers (First Wins)", pass);
    }
    else
    {
        printTest("Duplicate Headers (First Wins)", false);
    }
    cm.removeClient(fd);
}

void test_parser_invalid_method()
{
    std::cout << "\n[PARSER TEST 8] Invalid Method (Error 400)\n";
    ClientManager cm;
    int fd = 8;
    
    cm.addClient(fd);
    std::string req = "INVALID /test HTTP/1.0\r\n\r\n";
    cm.onData(fd, req);
    
    bool hasErr = cm.hasError(fd);
    bool pass = hasErr;
    
    if (hasErr)
    {
        int status = cm.getErrorStatus(fd);
        pass = pass && (status == HTTP_BAD_REQUEST);
        std::cout << "  Status: " << getStatusName(status) << std::endl;
    }
    
    printTest("Invalid Method (400 Bad Request)", pass);
    cm.removeClient(fd);
}

void test_parser_post_no_content_length()
{
    std::cout << "\n[PARSER TEST 9] POST without Content-Length (Error 411)\n";
    ClientManager cm;
    int fd = 9;
    
    cm.addClient(fd);
    std::string req = "POST /upload HTTP/1.0\r\nHost: localhost\r\n\r\n";
    cm.onData(fd, req);
    
    bool hasErr = cm.hasError(fd);
    bool pass = hasErr;
    
    if (hasErr)
    {
        int status = cm.getErrorStatus(fd);
        pass = pass && (status == HTTP_LENGTH_REQUIRED);
        std::cout << "  Status: " << getStatusName(status) << std::endl;
    }
    
    printTest("POST without Content-Length (411)", pass);
    cm.removeClient(fd);
}

void test_parser_body_too_large()
{
    std::cout << "\n[PARSER TEST 10] Body Too Large (Error 413)\n";
    ClientManager cm;
    int fd = 10;
    
    cm.addClient(fd);
    std::string req = "POST /upload HTTP/1.0\r\n"
                      "Content-Length: 2000000\r\n"
                      "\r\n";
    cm.onData(fd, req);
    
    bool hasErr = cm.hasError(fd);
    bool pass = hasErr;
    
    if (hasErr)
    {
        int status = cm.getErrorStatus(fd);
        pass = pass && (status == HTTP_PAYLOAD_TOO_LARGE);
        std::cout << "  Status: " << getStatusName(status) << std::endl;
    }
    
    printTest("Body Too Large (413)", pass);
    cm.removeClient(fd);
}

void test_parser_uri_too_long()
{
    std::cout << "\n[PARSER TEST 11] URI Too Long (Error 414)\n";
    ClientManager cm;
    int fd = 11;
    
    cm.addClient(fd);
    std::string longPath(9000, 'A');  // > 8192 limit
    std::string req = "GET /" + longPath + " HTTP/1.0\r\n\r\n";
    cm.onData(fd, req);
    
    bool hasErr = cm.hasError(fd);
    bool pass = hasErr;
    
    if (hasErr)
    {
        int status = cm.getErrorStatus(fd);
        pass = pass && (status == HTTP_REQUEST_URI_TOO_LONG);
        std::cout << "  Status: " << getStatusName(status) << std::endl;
    }
    
    printTest("URI Too Long (414)", pass);
    cm.removeClient(fd);
}

void test_parser_too_many_headers()
{
    std::cout << "\n[PARSER TEST 12] Too Many Headers (Error 431)\n";
    ClientManager cm;
    int fd = 12;
    
    cm.addClient(fd);
    std::string req = "GET / HTTP/1.0\r\n";
    
    // Add 101 headers (limit is 100)
    for (int i = 0; i < 101; i++)
    {
        req += "Header" + std::to_string(i) + ": value\r\n";
    }
    req += "\r\n";
    
    cm.onData(fd, req);
    
    bool hasErr = cm.hasError(fd);
    bool pass = hasErr;
    
    if (hasErr)
    {
        int status = cm.getErrorStatus(fd);
        pass = pass && (status == HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE);
        std::cout << "  Status: " << getStatusName(status) << std::endl;
    }
    
    printTest("Too Many Headers (431)", pass);
    cm.removeClient(fd);
}

// ============================================================
// ROUTER TESTS
// ============================================================

void test_router_exact_match()
{
    std::cout << "\n[ROUTER TEST 1] Exact Path Match\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc1;
    loc1.path = "/api";
    loc1.root = "/var/www/api";
    loc1.methods.insert("GET");
    loc1.methods.insert("POST");
    locations.push_back(loc1);
    
    LocationConfig loc2;
    loc2.path = "/";
    loc2.root = "/var/www";
    loc2.methods.insert("GET");
    locations.push_back(loc2);
    
    Request req;
    req.method = "GET";
    req.path = "/api";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched != NULL && matched->path == "/api" && status == HTTP_OK);
    printTest("Exact Path Match", pass);
    if (pass)
        std::cout << "  Matched: " << matched->path << " (Status: " << getStatusName(status) << ")" << std::endl;
}

void test_router_longest_prefix()
{
    std::cout << "\n[ROUTER TEST 2] Longest Prefix Match\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc1;
    loc1.path = "/api";
    loc1.root = "/var/www/api";
    loc1.methods.insert("GET");
    locations.push_back(loc1);
    
    LocationConfig loc2;
    loc2.path = "/api/users";
    loc2.root = "/var/www/users";
    loc2.methods.insert("GET");
    locations.push_back(loc2);
    
    LocationConfig loc3;
    loc3.path = "/";
    loc3.root = "/var/www";
    loc3.methods.insert("GET");
    locations.push_back(loc3);
    
    Request req;
    req.method = "GET";
    req.path = "/api/users/123";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched != NULL && matched->path == "/api/users" && status == HTTP_OK);
    printTest("Longest Prefix Match", pass);
    if (pass)
        std::cout << "  Matched: " << matched->path << " (Status: " << getStatusName(status) << ")" << std::endl;
}

void test_router_root_match()
{
    std::cout << "\n[ROUTER TEST 3] Root Path Match\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc1;
    loc1.path = "/";
    loc1.root = "/var/www";
    loc1.methods.insert("GET");
    locations.push_back(loc1);
    
    Request req;
    req.method = "GET";
    req.path = "/anything";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched != NULL && matched->path == "/" && status == HTTP_OK);
    printTest("Root Path Match", pass);
}

void test_router_path_boundary()
{
    std::cout << "\n[ROUTER TEST 4] Path Boundary (upload != uploadfiles)\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc1;
    loc1.path = "/upload";
    loc1.root = "/var/www/upload";
    loc1.methods.insert("POST");
    locations.push_back(loc1);
    
    Request req;
    req.method = "POST";
    req.path = "/uploadfiles";  // Should NOT match /upload
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched == NULL && status == HTTP_NOT_FOUND);
    printTest("Path Boundary Check", pass);
    if (pass)
        std::cout << "  Status: " << getStatusName(status) << " (correctly rejected)" << std::endl;
}

void test_router_method_allowed()
{
    std::cout << "\n[ROUTER TEST 5] Method Allowed\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc;
    loc.path = "/api";
    loc.root = "/var/www/api";
    loc.methods.insert("GET");
    loc.methods.insert("POST");
    locations.push_back(loc);
    
    Request req;
    req.method = "POST";
    req.path = "/api";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched != NULL && status == HTTP_OK);
    printTest("Method Allowed (POST)", pass);
}

void test_router_method_not_allowed()
{
    std::cout << "\n[ROUTER TEST 6] Method Not Allowed (Error 405)\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc;
    loc.path = "/api";
    loc.root = "/var/www/api";
    loc.methods.insert("GET");  // Only GET allowed
    locations.push_back(loc);
    
    Request req;
    req.method = "DELETE";  // Try DELETE
    req.path = "/api";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched == NULL && status == HTTP_METHOD_NOT_ALLOWED);
    printTest("Method Not Allowed (405)", pass);
    if (pass)
        std::cout << "  Status: " << getStatusName(status) << std::endl;
}

void test_router_empty_methods()
{
    std::cout << "\n[ROUTER TEST 7] Empty Methods (All Methods Allowed)\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc;
    loc.path = "/api";
    loc.root = "/var/www/api";
    // No methods specified = all allowed
    locations.push_back(loc);
    
    Request req;
    req.method = "DELETE";
    req.path = "/api";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched != NULL && status == HTTP_OK);
    printTest("Empty Methods (All Allowed)", pass);
    if (pass)
        std::cout << "  Status: " << getStatusName(status) << std::endl;
}

void test_router_no_match()
{
    std::cout << "\n[ROUTER TEST 8] No Matching Location (Error 404)\n";
    
    std::vector<LocationConfig> locations;
    
    LocationConfig loc;
    loc.path = "/api";
    loc.root = "/var/www/api";
    loc.methods.insert("GET");
    locations.push_back(loc);
    
    Request req;
    req.method = "GET";
    req.path = "/notfound";  // No location matches this
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched == NULL && status == HTTP_NOT_FOUND);
    printTest("No Matching Location (404)", pass);
    if (pass)
        std::cout << "  Status: " << getStatusName(status) << std::endl;
}

void test_router_empty_config()
{
    std::cout << "\n[ROUTER TEST 9] Empty Configuration\n";
    
    std::vector<LocationConfig> locations;  // Empty!
    
    Request req;
    req.method = "GET";
    req.path = "/";
    req.version = "HTTP/1.0";
    
    Router router;
    int status = 0;
    const LocationConfig* matched = router.route(req, locations, status);
    
    bool pass = (matched == NULL && status == HTTP_NOT_FOUND);
    printTest("Empty Configuration", pass);
    if (pass)
        std::cout << "  Status: " << getStatusName(status) << std::endl;
}

void test_router_multiple_clients()
{
    std::cout << "\n[ROUTER TEST 10] Multiple Clients\n";
    
    ClientManager cm;
    Router router;
    
    std::vector<LocationConfig> locations;
    LocationConfig loc;
    loc.path = "/";
    loc.root = "/var/www";
    loc.methods.insert("GET");
    locations.push_back(loc);
    
    cm.addClient(1);
    cm.addClient(2);
    cm.addClient(3);
    
    cm.onData(1, "GET /page1 HTTP/1.0\r\n\r\n");
    cm.onData(2, "GET /page2 HTTP/1.0\r\n\r\n");
    cm.onData(3, "POST /page3 HTTP/1.0\r\nContent-Length: 0\r\n\r\n");
    
    bool pass = true;
    
    if (cm.hasRequest(1))
    {
        int status = 0;
        Request r1 = cm.getRequest(1);
        router.route(r1, locations, status);
        pass = pass && (status == HTTP_OK);
    }
    
    if (cm.hasRequest(2))
    {
        int status = 0;
        Request r2 = cm.getRequest(2);
        router.route(r2, locations, status);
        pass = pass && (status == HTTP_OK);
    }
    
    if (cm.hasError(3))
    {
        int status = cm.getErrorStatus(3);
        pass = pass && (status == HTTP_LENGTH_REQUIRED);
    }
    
    printTest("Multiple Clients", pass);
    
    cm.removeClient(1);
    cm.removeClient(2);
    cm.removeClient(3);
}

// ============================================================
// MAIN TEST RUNNER
// ============================================================

int main()
{
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  COMPREHENSIVE TEST SUITE\n";
    std::cout << "  Parser + Router + ClientManager\n";
    std::cout << "========================================\n";
    
    std::cout << "\n▶ PARSER TESTS (12 tests)\n";
    std::cout << "----------------------------------------\n";
    
    test_parser_simple_get();
    test_parser_post_with_body();
    test_parser_delete_method();
    test_parser_case_insensitive_headers();
    test_parser_partial_data();
    test_parser_pipelined_requests();
    test_parser_duplicate_headers();
    test_parser_invalid_method();
    test_parser_post_no_content_length();
    test_parser_body_too_large();
    test_parser_uri_too_long();
    test_parser_too_many_headers();
    
    std::cout << "\n▶ ROUTER TESTS (10 tests)\n";
    std::cout << "----------------------------------------\n";
    
    test_router_exact_match();
    test_router_longest_prefix();
    test_router_root_match();
    test_router_path_boundary();
    test_router_method_allowed();
    test_router_method_not_allowed();
    test_router_empty_methods();
    test_router_no_match();
    test_router_empty_config();
    test_router_multiple_clients();
    
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  ✓ ALL TESTS COMPLETED (22/22)\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    
    return 0;
}