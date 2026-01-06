#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "ListenerSocket.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>

class RecvBuffer;

struct HttpRequest{
    std::string method;
    std::string target;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;
};


#endif