#include <gtest/gtest.h>
#include <string.h>
#include "RecvBuffer.h"
#include "MessageFramer.h"

static void WriteAll(RecvBuffer& rb, const void* data, size_t len)
{
    size_t written = 0;
    auto err = rb.Write(data, len, written);
    ASSERT_EQ(err, RecvBuf_Ok);
    ASSERT_EQ(written, len);
}

TEST(MessageFramer, SplitHeaderThenPayload)
{
    RecvBuffer rb(1024);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    const char payload[] = "HELLO";
    std::vector<std::uint8_t> encoded;
    ASSERT_EQ(MessageFramer::Encode(payload, sizeof(payload)-1, encoded), eFrameError::Framer_Ok);

    // 헤더 2바이트 + 2바이트 + payload로 쪼개서 입력
    WriteAll(rb, encoded.data(), 2);
    Frame f;
    EXPECT_EQ(MessageFramer::PopFrame(rb, f), eFrameError::Framer_NeedMore);

    WriteAll(rb, encoded.data() + 2, 2);
    EXPECT_EQ(MessageFramer::PopFrame(rb, f), eFrameError::Framer_NeedMore);

    WriteAll(rb, encoded.data() + 4, encoded.size() - 4);
    EXPECT_EQ(MessageFramer::PopFrame(rb, f), eFrameError::Framer_Ok);

    ASSERT_EQ(f.payload.size(), sizeof(payload)-1);
    EXPECT_EQ(::memcmp(f.payload.data(), payload, sizeof(payload)-1), 0);
}

TEST(MessageFramer, MultipleFramesInOneChunk)
{
    RecvBuffer rb(4096);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    std::vector<std::uint8_t> a, b, chunk;
    ASSERT_EQ(MessageFramer::Encode("A", 1, a), eFrameError::Framer_Ok);
    ASSERT_EQ(MessageFramer::Encode("BC", 2, b), eFrameError::Framer_Ok);

    chunk.reserve(a.size() + b.size());
    chunk.insert(chunk.end(), a.begin(), a.end());
    chunk.insert(chunk.end(), b.begin(), b.end());

    WriteAll(rb, chunk.data(), chunk.size());

    Frame f1, f2;
    EXPECT_EQ(MessageFramer::PopFrame(rb, f1), eFrameError::Framer_Ok);
    EXPECT_EQ(MessageFramer::PopFrame(rb, f2), eFrameError::Framer_Ok);
    EXPECT_EQ(MessageFramer::PopFrame(rb, f2), eFrameError::Framer_NeedMore);

    ASSERT_EQ(f1.payload.size(), 1u);
    ASSERT_EQ(f2.payload.size(), 2u);
    EXPECT_EQ(f1.payload[0], 'A');
    EXPECT_EQ(f2.payload[0], 'B');
    EXPECT_EQ(f2.payload[1], 'C');
}

TEST(MessageFramer, NeedMoreWhenOnlyLengthPresent)
{
    RecvBuffer rb(1024);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    // length=10 헤더만 넣고 payload 없음
    std::uint8_t hdr[4] = {0,0,0,10};
    WriteAll(rb, hdr, 4);

    Frame f;
    EXPECT_EQ(MessageFramer::PopFrame(rb, f), eFrameError::Framer_NeedMore);
}

TEST(MessageFramer, OverflowWhenLengthTooLarge)
{
    RecvBuffer rb(1024);
    ASSERT_EQ(rb.Open(), RecvBuf_Ok);

    // length = kMaxPayload + 1
    const uint32_t big = MessageFramer::kMaxPayload + 1;
    std::uint8_t hdr[4] = {
        (std::uint8_t)((big >> 24) & 0xFF),
        (std::uint8_t)((big >> 16) & 0xFF),
        (std::uint8_t)((big >> 8) & 0xFF),
        (std::uint8_t)(big & 0xFF),
    };
    WriteAll(rb, hdr, 4);

    Frame f;
    EXPECT_EQ(MessageFramer::PopFrame(rb, f), eFrameError::Framer_Overflow);
}
