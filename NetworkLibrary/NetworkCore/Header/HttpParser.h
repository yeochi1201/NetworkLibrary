#ifndef HTTP_PARSER
#define HTTP_PARSER


#include <string>
#include <string_view>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpTypes.h"
#include "ListenerSocket.h"

class RecvBuffer;

class HttpParser{
public:
    enum class Result {Http_Ok, Http_NeedMore, Http_Error};
    
    HttpParser();
    
    Result TryParse(RecvBuffer& rb, HttpRequest& rq, std::string* outErr = nullptr);
    void Reset();

private:
    enum class State {Http_RequestLine, Http_Headers, Http_Body};
    
    std::string mBuf;
    State mState = State::Http_RequestLine;
    HttpRequest mCur;
    std::size_t mContentLength = 0;

private:
    static std::string ToLower(std::string_view s);
    static std::string Trim(std::string_view s);
    static bool SplitOnce(std::string_view s, char delim, std::string_view& left, std::string_view& right);
    
    bool PullFromRecvBuffer(RecvBuffer& rb, std::size_t maxPull = 64 * 1024);
    bool PopLine(std::string& outLine);

    bool ParseRequestLine(const std::string& line, std::string* err);
    bool ParseHeaderLine(const std::string& line, std::string* err);
    

    HttpMethod ParseMethod(std::string_view m);
    HttpVersion ParseVersion(std::string_view v);
    void ParseTarget(HttpRequest& req);
    void ParseQueryString(std::string_view query, QueryMap& out);

};
#endif