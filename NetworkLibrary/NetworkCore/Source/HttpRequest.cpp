#include "HttpRequest.h"

#include <algorithm>
#include <cctype>

void HttpRequest::Clear(){
    method = HttpMethod::Unknown;
    methodText.clear();

    target.clear();
    path.clear();
    queryString.clear();

    version = HttpVersion::Unknown;
    versionText.clear();

    headers.clear();
    queryParams.clear();
    body.clear();

    keepAlive = true;
}

std::optional<std::string_view> HttpRequest::Header(std::string_view key) const {
    std::string normalized;
    normalized.reserve(key.size());

    for(char c : key){
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    auto it = headers.find(normalized);
    if(it == headers.end()) return std::nullopt;

    return std::string_view(it->second);
}

bool HttpRequest::HasHeader(std::string_view key) const {
    return Header(key).has_value();
}

std::string_view HttpRequest::BodyText() const{
    if(body.empty()) return {};
    return std::string_view(reinterpret_cast<const char*>(body.data()), body.size());
}