#ifndef HTTP_HEADER_UTIL
#define HTTP_HEADER_UTIL

#include <string>
#include <optional>

#include "HttpTypes.h"

std::string NormalizeHeaderKey(std::string_view key);

std::string_view HeaderName(HttpHeader h);
std::optional<HttpHeader> ParseHeaderName(std::string_view key);

std::optional<std::string_view> FindHeader(const HeaderMap& headers, std::string_view key);
bool HasHeader(const HeaderMap& headers, std::string_view key);

void SetHeader(HeaderMap& headers, std::string_view key, std::string value);
bool EraseHeader(HeaderMap& headers, std::string_view key);

#endif