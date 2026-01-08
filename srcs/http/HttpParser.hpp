#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "shared/Request.hpp"
#include <string>
#include <sstream>

class HttpParser {
public:
    HttpParser();
    void feed(const std::string& chunk); //add data, tries parsing base on state
    bool isComplete() const; // when state == COMPLETE
    bool hasError() const; // when bad method/version
    const Request& getRequest() const; // returns parsed request, read only
    void reset(); //clear everything, used for keep alive/ next request on same fd
private:
    enum State 
    { REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
    };

    State state;
    std::string buffer; //recv() gives partial lines
    Request request; //holds method, path, version, headers, body

    bool parseRequestLine();
    bool parseHeaders();
    bool parseBody();
    bool validateMethod();
    bool validateVersion();
};

std::string trim(const std::string& str);

#endif