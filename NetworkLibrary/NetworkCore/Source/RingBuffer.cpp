#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t bufSize)
    : mBufSize(bufSize), mBuf(bufSize ? std::make_unique<std::uint8_t[]>(bufSize) : nullptr), mIsFull(false), mReadPos(0), mWritePos(0)
{
}

RingBuffer::RingBuffer(RingBuffer&& other) noexcept
    : mBuf(std::move(other.mBuf)), mBufSize(other.mBufSize), mReadPos(other.mReadPos), mWritePos(other.mWritePos), mIsFull(other.mIsFull)
{
    other.mBufSize = 0;
    other.mReadPos  = 0;
    other.mWritePos = 0;
    other.mIsFull   = false;
}

RingBuffer& RingBuffer::operator=(RingBuffer&& other) noexcept
{
    if (this != &other) {
        mBuf       = std::move(other.mBuf);
        mBufSize  = other.mBufSize;
        mReadPos   = other.mReadPos;
        mWritePos  = other.mWritePos;
        mIsFull    = other.mIsFull;

        other.mBufSize = 0;
        other.mReadPos  = 0;
        other.mWritePos = 0;
        other.mIsFull   = false;
    }
    return *this;
}