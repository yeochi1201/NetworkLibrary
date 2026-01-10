#include <gtest/gtest.h>
#include "RecvBuffer.h"
#include "HttpParser.h"

static void WriteAll(RecvBuffer& rb, const char* s)
{
    std::size_t written = 0;
    ASSERT_EQ(rb.Write(s, ::strlen(s), written), RecvBuf_Ok);
    ASSERT_EQ(written, ::strlen(s));
}

TEST(HttpParser, ParseSimpleGet)
{
    RecvBuffer rb(4096);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    HttpParser p;
    HttpRequest req;

    WriteAll(rb,
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n");

    auto r = p.TryParse(rb, req);
    EXPECT_EQ(r, HttpParser::Result::Http_Ok);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.target, "/health");
    EXPECT_EQ(req.version, "HTTP/1.1");
}

TEST(HttpParser, RequestLineSplit)
{
    RecvBuffer rb(4096);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    HttpParser p;
    HttpRequest req;

    WriteAll(rb, "GET /he");
    EXPECT_EQ(p.TryParse(rb, req), HttpParser::Result::Http_NeedMore);

    WriteAll(rb, "alth HTTP/1.1\r\nHost: x\r\n\r\n");
    EXPECT_EQ(p.TryParse(rb, req), HttpParser::Result::Http_Ok);
    EXPECT_EQ(req.target, "/health");
}

TEST(HttpParser, PostWithBodySplit)
{
    RecvBuffer rb(4096);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    HttpParser p;
    HttpRequest req;

    WriteAll(rb,
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\nhe");
    EXPECT_EQ(p.TryParse(rb, req), HttpParser::Result::Http_NeedMore);

    WriteAll(rb, "llo");
    EXPECT_EQ(p.TryParse(rb, req), HttpParser::Result::Http_Ok);

    ASSERT_EQ(req.body.size(), 5u);
    EXPECT_EQ(std::string((char*)req.body.data(), req.body.size()), "hello");
}

TEST(HttpParser, TwoRequestsBackToBack)
{
    RecvBuffer rb(4096);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    HttpParser p;
    HttpRequest req1, req2;

    WriteAll(rb,
        "GET /health HTTP/1.1\r\nHost: a\r\n\r\n"
        "GET /echo?msg=hi HTTP/1.1\r\nHost: a\r\n\r\n");

    EXPECT_EQ(p.TryParse(rb, req1), HttpParser::Result::Http_Ok);
    EXPECT_EQ(req1.target, "/health");

    EXPECT_EQ(p.TryParse(rb, req2), HttpParser::Result::Http_Ok);
    EXPECT_EQ(req2.target, "/echo?msg=hi");
}

TEST(HttpParser, BadHeader)
{
    RecvBuffer rb(4096);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    HttpParser p;
    HttpRequest req;
    std::string err;

    WriteAll(rb,
        "GET / HTTP/1.1\r\n"
        "Host localhost\r\n" // ':' 없음
        "\r\n");

    auto r = p.TryParse(rb, req, &err);
    EXPECT_EQ(r, HttpParser::Result::Http_Error);
    EXPECT_FALSE(err.empty());
}
