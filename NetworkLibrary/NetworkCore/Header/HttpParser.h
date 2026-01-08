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

    void Clear(){
        method.clear();
        target.clear();
        version.clear();
        headers.clear();
        body.clear();
    }

    std::optional<std::string_view> Header(std::string_view key) const{
        std:: string k(key);
        for(auto&c : k) c = (char)std::tolower((unsigned char)c);

        auto it = headers.find(k);
        if (it == headers.end()) return std::nullopt;

        return std::string_view(it->second);
    }
};

struct HttpResponse{
    int status = 200;
    std::string reason = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;

    void SetTextBody(std::string_view s){
        body.assign(s.begin(), s.end());
        headers["Content-Type"] = "text/plain; charset=utf-8";
    }
};
#endif