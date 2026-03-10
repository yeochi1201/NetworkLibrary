#include "HttpHeaderUtils.h"

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

std::string_view HeaderName(HttpHeader h){
    switch(h){
    case HttpHeader::Host:
        return "host";
    case HttpHeader::Connection:
        return "connection";
    case HttpHeader::ContentLength:
        return "content-length";
    case HttpHeader::ContentType:
        return "content-type";
    case HttpHeader::UserAgent:
        return "user-agent";
    case HttpHeader::Accept:
        return "accept";
    case HttpHeader::Authorization:
        return "authorization";
    default:
        return "";
    }
}

std::optional<HttpHeader> ParseHeaderName(std::string_view key)
{
    const std::string normalized = NormalizeHeaderKey(key);

    if (normalized == "host")
        return HttpHeader::Host;
    if (normalized == "connection")
        return HttpHeader::Connection;
    if (normalized == "content-length")
        return HttpHeader::ContentLength;
    if (normalized == "content-type")
        return HttpHeader::ContentType;
    if (normalized == "user-agent")
        return HttpHeader::UserAgent;
    if (normalized == "accept")
        return HttpHeader::Accept;
    if (normalized == "authorization")
        return HttpHeader::Authorization;

    return std::nullopt;
}

std::optional<std::string_view> FindHeader(const HeaderMap& headers, std::string_view key)
{
    const std::string normalized = NormalizeHeaderKey(key);

    auto it = headers.find(normalized);
    if (it == headers.end())
    {
        return std::nullopt;
    }

    return std::string_view(it->second);
}

bool HasHeader(const HeaderMap& headers, std::string_view key)
{
    return FindHeader(headers, key).has_value();
}

void SetHeader(HeaderMap& headers, std::string_view key, std::string value)
{
    headers[NormalizeHeaderKey(key)] = std::move(value);
}

bool EraseHeader(HeaderMap& headers, std::string_view key)
{
    return headers.erase(NormalizeHeaderKey(key)) > 0;
}