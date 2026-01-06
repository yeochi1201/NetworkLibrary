#include <gtest/gtest.h>
#include "SendBuffer.h"

static void SBWriteAll(SendBuffer& sb, const void* data, size_t len)
{
    size_t written = 0;
    auto err = sb.Write(data, len, written);
    ASSERT_EQ(err, SendBuf_Ok);
    ASSERT_EQ(written, len);
}

TEST(SendBuffer, PeekDoesNotConsume)
{
    SendBuffer sb(64);
    ASSERT_EQ(sb.Open(), SendBuf_Ok);

    SBWriteAll(sb, "HELLO", 5);

    std::uint8_t tmp[8]{};
    size_t peeked = 0;
    ASSERT_EQ(sb.Peek(tmp, sizeof(tmp), peeked), SendBuf_Ok);
    ASSERT_EQ(peeked, 5u);
    EXPECT_EQ(::memcmp(tmp, "HELLO", 5), 0);

    // 아직 소비되지 않았으니 Read 하면 동일 데이터가 나와야 함
    size_t read = 0;
    ASSERT_EQ(sb.Read(tmp, sizeof(tmp), read), SendBuf_Ok);
    ASSERT_EQ(read, 5u);
    EXPECT_EQ(::memcmp(tmp, "HELLO", 5), 0);
}

TEST(SendBuffer, PartialConsumeMaintainsOrder)
{
    SendBuffer sb(64);
    ASSERT_EQ(sb.Open(), SendBuf_Ok);

    SBWriteAll(sb, "ABCDEFG", 7);

    // Peek 7바이트 후 3바이트만 Consume
    std::uint8_t tmp[16]{};
    size_t peeked = 0;
    ASSERT_EQ(sb.Peek(tmp, sizeof(tmp), peeked), SendBuf_Ok);
    ASSERT_EQ(peeked, 7u);

    ASSERT_EQ(sb.Consume(3), SendBuf_Ok);

    size_t read = 0;
    ASSERT_EQ(sb.Read(tmp, sizeof(tmp), read), SendBuf_Ok);
    ASSERT_EQ(read, 4u);
    EXPECT_EQ(::memcmp(tmp, "DEFG", 4), 0);
}
