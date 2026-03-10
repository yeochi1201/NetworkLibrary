#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "HttpTypes.h"

class HttpParser;

class HttpRequest{
    friend class HttpParser;
public:
    void Clear();

    std::optional<std::string_view> Header(std::string_view key) const;
    bool HasHeader(std::string_view) const;
    std::string_view BodyText() const;
private:
    HttpMethod method = HttpMethod::Unknown;
    std::string methodText;

    std::string target;
    std::string path;
    std::string queryString;

    HttpVersion version = HttpVersion::Unknown;
    std::string versionText;

    HeaderMap headers;
    QueryMap queryParams;
    
    std::vector<std::uint8_t> body;

    bool keepAlive = true;
};

#endif