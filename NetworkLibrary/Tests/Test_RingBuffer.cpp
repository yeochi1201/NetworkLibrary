#include <gtest/gtest.h>
#include "RingBuffer.h"

TEST(RingBuffer, WritePeekConsumeReadBasic)
{
    RingBuffer rb(16);
    rb.Reset();

    const char* msg = "ABCDE";
    EXPECT_EQ(rb.Write(msg, 5), 5u);

    char tmp[8]{};
    EXPECT_EQ(rb.Peek(tmp, 3), 3u);
    EXPECT_EQ(::memcmp(tmp, "ABC", 3), 0);

    rb.Consume(2);

    char out[8]{};
    EXPECT_EQ(rb.Read(out, 3), 3u);
    EXPECT_EQ(::memcmp(out, "CDE", 3), 0);

    EXPECT_TRUE(rb.IsEmpty());
}

TEST(RingBuffer, WrapAroundKeepsOrder)
{
    RingBuffer rb(8);
    rb.Reset();

    EXPECT_EQ(rb.Write("123456", 6), 6u);

    char out1[4]{};
    EXPECT_EQ(rb.Read(out1, 4), 4u);
    EXPECT_EQ(::memcmp(out1, "1234", 4), 0);

    // wrap 유도
    EXPECT_EQ(rb.Write("ABCD", 4), 4u);

    char out2[6]{};
    EXPECT_EQ(rb.Read(out2, 6), 6u);
    EXPECT_EQ(::memcmp(out2, "56ABCD", 6), 0);
}
