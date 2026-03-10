#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <utility>

std::string NormalizeHeaderKey(std::string_view key){
    std::string out;
    out.reserve(key.size());

    for(char c : key){
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    return out;
}

void HttpResponse::Clear(){
    status = 200;
    reason = "OK";
    headers.clear();
    body.clear();
}

void HttpResponse::SetTextBody(std::string_view s)
{
    body.assign(s.begin(), s.end());
    headers["content-type"] = "text/plain; charset=utf-8";
}

void HttpResponse::SetJsonBody(std::string_view s)
{
    body.assign(s.begin(), s.end());
    headers["content-type"] = "application/json; charset=utf-8";
}

void HttpResponse::SetBody(const std::vector<std::uint8_t>& bytes)
{
    body = bytes;
}

void HttpResponse::SetBody(std::vector<std::uint8_t>&& bytes)
{
    body = std::move(bytes);
}

void HttpResponse::SetHeader(std::string key, std::string value)
{
    headers[NormalizeHeaderKey(key)] = std::move(value);
}

bool HttpResponse::HasHeader(std::string_view key) const
{
    return headers.find(NormalizeHeaderKey(key)) != headers.end();
}