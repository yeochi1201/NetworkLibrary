#include "HttpResponse.h"

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

std::vector<std::uint8_t> HttpResponse::Serialize(bool keepAlive) const{
    std::string header;
    header.reserve(256);

    header += "HTTP/1.1 ";
    header += std::to_string(status);
    header += " ";
    header += reason;
    header += "\r\n";

    if (!HasHeader("content-length"))
    {
        header += "Content-Length: ";
        header += std::to_string(body.size());
        header += "\r\n";
    }

    if (!HasHeader("connection"))
    {
        header += "Connection: ";
        header += (keepAlive ? "keep-alive" : "close");
        header += "\r\n";
    }

    for (const auto& [k, v] : headers)
    {
        header += k;
        header += ": ";
        header += v;
        header += "\r\n";
    }

    header += "\r\n";

    std::vector<std::uint8_t> out;
    out.reserve(header.size() + body.size());

    out.insert(out.end(), header.begin(), header.end());
    out.insert(out.end(), body.begin(), body.end());

    return out;
}