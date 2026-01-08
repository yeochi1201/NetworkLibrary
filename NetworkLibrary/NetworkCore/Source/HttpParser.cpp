#include "HttpParser.h"
#include "RecvBuffer.h"
#include <cctype>
#include <cstring>
#include <cstdlib>

HttpParser::HttpParser() = default;

void HttpParser::Reset(){
    mBuf.clear();
    mState = State::Http_RequestLine;
    mCur.Clear();
    mContentLength = 0;
}

std::string HttpParser::ToLower(std::string_view s){
    std::string out;
    out.reserve(s.size());

    for(char c : s) out.push_back((char)std::tolower((unsigned char)c));
}