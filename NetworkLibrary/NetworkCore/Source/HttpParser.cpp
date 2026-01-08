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