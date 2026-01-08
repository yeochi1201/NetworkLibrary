#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "ListenerSocket.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>

class RecvBuffer;

struct HttpRequest{
    std::string method;
    std::string target;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;

    void Clear(){
        method.clear();
        target.clear();
        version.clear();
        headers.clear();
        body.clear();
    }

    std::optional<std::string_view> Header(std::string_view key) const{
        std:: string k(key);
        for(auto&c : k) c = (char)std::tolower((unsigned char)c);

        auto it = headers.find(k);
        if (it == headers.end()) return std::nullopt;

        return std::string_view(it->second);
    }
};

struct HttpResponse{
    int status = 200;
    std::string reason = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;

    void SetTextBody(std::string_view s){
        body.assign(s.begin(), s.end());
        headers["Content-Type"] = "text/plain; charset=utf-8";
    }
};
std::vector<std::uint8_t> BuildHttpResponseBytes(const HttpResponse& resp, bool keepAlive);

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
    bool PraseHeaderLine(const std::string& line, std::string* err);
};
#endif