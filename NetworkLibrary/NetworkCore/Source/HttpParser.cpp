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

    return out;
}

std::string HttpParser::Trim(std::string_view s){
    std::size_t b = 0;
    std::size_t e = s.size();

    while (b < e && std::isspace((unsigned char)s[b])) b++;
    while (e > b && std::isspace((unsigned char)s[e-1])) e--;

    return std::string(s.substr(b, e-b));
}

bool HttpParser::SplitOnce(std::string_view s, char delim, std::string_view& left, std::string_view& right){
    auto pos = s.find(delim);
    if(pos == std::string_view::npos) return false;

    left = s.substr(0, pos);
    right = s.substr(pos+1);

    return true;
}

bool HttpParser::PullFromRecvBuffer(RecvBuffer& rb, std::size_t maxPull){
    std::size_t  available = rb.WriteSpace();
    if(available == 0) return false;

    if(available > maxPull) available = maxPull;

    std::string tmp;
    tmp.resize(available);

    std::size_t peeked = 0;
    if(rb.Peek(tmp.data(), available, peeked)!= RecvBuf_Ok || peeked == 0) return false;

    mBuf.append(tmp.data(), peeked);
    if(rb.Consume(peeked) != RecvBuf_Ok) return false;

    return true;
}

bool HttpParser::PopLine(std::string& outLine){
    auto pos = mBuf.find("\r\n");
    if(pos == std::string::npos) return false;

    outLine = mBuf.substr(0, pos);
    mBuf.erase(0, pos+2);

    return true;
}

bool HttpParser::ParseRequestLine(const std::string& line, std::string* err){
    std::size_t p1 = line.find(' ');
    if(p1 == std::string::npos) {if(err) *err = "Bad Request Line"; return false;}

    std::size_t p2 = line.find(' ', p1+1);
    if(p2 == std::string::npos) {if(err) *err = "Bad Request Line"; return false;}

    mCur.method = line.substr(0, p1);
    mCur.target = line.substr(p1+1, p2-(p1+1));
    mCur.version = line.substr(p2+1);

    if(mCur.method.empty() || mCur.target.empty() || mCur.version.empty()){
        if(err) *err = "Bad Request Line"; return false;
    }

    if(mCur.version != "HTTP/1.1" && mCur.version != "HTTP/1.0"){
        if(err) *err = "Unsupported HTTP version"; return false;
    }

    return true;
}