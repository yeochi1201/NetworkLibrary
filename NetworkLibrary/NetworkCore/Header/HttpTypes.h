#ifndef HTTP_TYPES
#define HTTP_TYPES

#include <string>
#include <unordered_map>

enum class HttpMethod{
    Get,
    Post,
    Put,
    Delete,
    Patch,
    Head,
    Options,
    Unknown
};

enum class HttpVersion{
    Http10,
    Http11,
    Unknown
};

using HeaderMap = std::unordered_map<std::string, std::string>;
using QueryMap = std::unordered_map<std::string, std::string>;

#endif