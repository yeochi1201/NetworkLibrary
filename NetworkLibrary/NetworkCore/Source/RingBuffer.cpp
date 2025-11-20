#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t bufSize)
    : mBufSize(bufSize), mBuf(bufSize ? std::make_unique<std::uint8_t[]>(bufSize) : nullptr), mIsFull(false), mReadPos(0), mWritePos(0)
{
}

RingBuffer::~RingBuffer()
{
    Close();
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

void RingBuffer::Reset()
{
    mReadPos  = 0;
    mWritePos = 0;
    mIsFull   = false;
}

void RingBuffer::Close()
{
    mReadPos  = 0;
    mWritePos = 0;
    mIsFull   = false;
    mBuf.reset();
}

size_t RingBuffer::BufSize() const noexcept
{
    return mBufSize;
}

size_t RingBuffer::WriteSpace() const noexcept
{
    if (mIsFull) {
        return mBufSize;
    }
    if (mWritePos >= mReadPos) {
        return mWritePos - mReadPos;
    } else {
        return mBufSize - (mReadPos - mWritePos);
    }
}
size_t RingBuffer::FreeSpace() const noexcept
{
    if (mIsFull) {
        return 0;
    }
    return mBufSize - WriteSpace();
}
bool RingBuffer::IsEmpty() const
{
    return !mIsFull && (mReadPos == mWritePos);
}

bool RingBuffer::IsFull() const
{
    return mIsFull;
}