#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "HttpTypes.h"

class HttpParser;
class HttpResponse{
    friend class HttpParser;
public:
    void Clear();

    void SetTextBody(std::string_view s);
    void SetJsonBody(std::string_view s);
    void SetBody(const std::vector<std::uint8_t>& bytes);
    void SetBody(std::vector<std::uint8_t>&& bytes);

    void SetHeader(std::string key, std::string value);
    bool HasHeader(std::string_view key) const;

    std::vector<std::uint8_t> Serialize(bool keepAlive) const;
private:
    int status = 200;
    std::string reason = "OK";
    
    HeaderMap headers;
    QueryMap queryParams;

    std::vector<std::uint8_t> body;
};

#endif