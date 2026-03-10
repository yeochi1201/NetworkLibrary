#include "HttpRequest.h"
#include "HttpHeaderUtils.h"

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
    return ::FindHeader(headers, key);
}

bool HttpRequest::HasHeader(std::string_view key) const {
    return ::HasHeader(headers, key);
}

std::string_view HttpRequest::BodyText() const{
    if(body.empty()) return {};
    return std::string_view(reinterpret_cast<const char*>(body.data()), body.size());
}