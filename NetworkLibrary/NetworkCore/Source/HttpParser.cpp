#include "HttpParser.h"
#include "RecvBuffer.h"
#include <cctype>
#include <cstddef>
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

bool HttpParser::ParseHeaderLine(const std::string& line, std::string* err){
    std::string_view left, right;
    if(!SplitOnce(line, ':', left, right)){
        if(err) *err = "Bad Header Line"; return false;
    }

    auto k = ToLower(Trim(left));
    auto v = Trim(right);
    
    if(k.empty()){
        if(err) *err = "Bad Header Key"; return false;
    }

    mCur.headers[k] = v;
    return true;
}

HttpParser::Result HttpParser::TryParse(RecvBuffer& rb, HttpRequest& rq, std::string* outErr){
    //Pulling
    if(rb.WriteSpace() > 0) (void)PullFromRecvBuffer(rb);

    while(true){
        if(mState == State::Http_RequestLine){
            std::string line;
            if(!PopLine(line)) return Result::Http_NeedMore;
            if(line.empty()) return Result::Http_NeedMore;
            if(!ParseRequestLine(line, outErr)) return Result::Http_Error;

            mState = State::Http_Headers;
            continue;
        }

        if(mState == State::Http_Headers){
            std::string line;
            if(!PopLine(line)) return Result::Http_NeedMore;
            if(line.empty()){
                mContentLength = 0;
                auto it = mCur.headers.find("content-length");
                if(it != mCur.headers.end()){
                    char* end = nullptr;
                    long v = std::strtol(it->second.c_str(), &end, 10);
                    if(end == it->second.c_str() || v < 0){
                        if(outErr) *outErr = "Invalid Content-Length";
                        return Result::Http_Error;
                    }
                    mContentLength = (std::size_t)v;
                }

                if(mContentLength == 0){
                    rq = std::move(mCur);
                    mCur.Clear();
                    mState = State::Http_RequestLine;
                    return Result::Http_Ok;
                }

                mState = State::Http_Body;
                continue;
            }
        }

        if(mState == State::Http_Body){
            if(mBuf.size() < mContentLength){
                if(rb.WriteSpace() > 0){
                    if(PullFromRecvBuffer(rb)) continue;
                }
                return Result::Http_NeedMore;
            }

            mCur.body.assign(mBuf.begin(), mBuf.begin()+ (std::ptrdiff_t)mContentLength);
            mBuf.erase(0, mContentLength);

            rq = std::move(mCur);
            mCur.Clear();
            mContentLength = 0;
            return Result::Http_Ok;
        }
    }
}