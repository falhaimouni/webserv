#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include "../shared/Request.hpp"
#include "../shared/HttpStatus.hpp"
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
    std::string trim(const std::string& str);
    int getErrorStatus() const;
    void processBuffer();
    long long getContentLength() const;
    size_t getBodySize() const;
    void forceError(int status);

private:
    static const size_t MAX_LINE_SIZE = 8192; // Max request line or header line
    static const size_t MAX_BODY_SIZE = 16777216; // 16MB default (allows testing various configs)
    static const size_t MAX_HEADERS = 100; // Max number of headers
    size_t headerCount;

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

    int errorStatus;
    bool isChunked;
    size_t chunkSize;

    bool parseRequestLine();
    bool parseHeaders();
    bool parseBody();
    bool validateMethod();
    bool validateVersion();
};


#endif