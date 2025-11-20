#include "RingBuffer.h"

#include <cstring>

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

size_t RingBuffer::DataSpace() const noexcept
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
    return mBufSize - DataSpace();
}
bool RingBuffer::IsEmpty() const
{
    return !mIsFull && (mReadPos == mWritePos);
}

bool RingBuffer::IsFull() const
{
    return mIsFull;
}

std::size_t RingBuffer::Read(void* dst, size_t len)
{
    if (!mBuf || !dst || len == 0 || mBufSize == 0) {
        return 0;
    }

    std::size_t available = DataSpace();
    if (available == 0) {
        return 0;
    }

    std::size_t toRead = (len < available) ? len : available;
    auto* out = static_cast<std::uint8_t*>(dst);

    std::size_t first    = toRead;
    std::size_t untilEnd = mBufSize - mReadPos;
    if (first > untilEnd) {
        first = untilEnd;
    }

    std::memcpy(out, mBuf.get() + mReadPos, first);
    mReadPos = (mReadPos + first) % mBufSize;

    std::size_t readBytes = first;

    std::size_t remain = toRead - first;
    if (remain > 0) {
        std::memcpy(out + first, mBuf.get() + mReadPos, remain);
        mReadPos = (mReadPos + remain) % mBufSize;
        readBytes += remain;
    }

    if (readBytes > 0) {
        mIsFull = false;
    }

    return readBytes;
}
std::size_t RingBuffer::Peek(void* dst, size_t len)
{
    if (!mBuf || !dst || len == 0 || mBufSize == 0) {
        return 0;
    }

    std::size_t available = DataSpace();
    if (available == 0) {
        return 0;
    }

    std::size_t toPeek = (len < available) ? len : available;
    auto* out = static_cast<std::uint8_t*>(dst);
    std::size_t readPos = mReadPos;

    std::size_t first    = toPeek;
    std::size_t untilEnd = mBufSize - readPos;
    if (first > untilEnd) {
        first = untilEnd;
    }

    std::memcpy(out, mBuf.get() + readPos, first);
    readPos = (readPos + first) % mBufSize;

    std::size_t readBytes = first;

    std::size_t remain = toPeek - first;
    if (remain > 0) {
        std::memcpy(out + first, mBuf.get() + readPos, remain);
        readPos = (readPos + remain) % mBufSize;
        readBytes += remain;
    }

    return readBytes;
}
void RingBuffer::Consume(size_t len)
{
   if (len == 0 || mBufSize == 0) {
        return;
    }

    std::size_t available = DataSpace();
    if (available == 0) {
        return;
    }

    if (len >= available) {
        Reset();
        return;
    }

    mReadPos = (mReadPos + len) % mBufSize;
    mIsFull  = false;
}
std::size_t RingBuffer::Write(const void* src, size_t len)
{
    if (!mBuf || !src || len == 0 || mBufSize == 0) {
        return 0;
    }

    std::size_t freeSpace = FreeSpace();
    if (freeSpace == 0) {
        return 0;
    }

    std::size_t toWrite = (len < freeSpace) ? len : freeSpace;
    auto* in = static_cast<const std::uint8_t*>(src);

    // 첫 번째 연속 구간
    std::size_t first    = toWrite;
    std::size_t untilEnd = mBufSize - mWritePos;
    if (first > untilEnd) {
        first = untilEnd;
    }

    std::memcpy(mBuf.get() + mWritePos, in, first);
    mWritePos = (mWritePos + first) % mBufSize;

    std::size_t written = first;

    std::size_t remain = toWrite - first;
    if (remain > 0) {
        std::memcpy(mBuf.get() + mWritePos, in + first, remain);
        mWritePos = (mWritePos + remain) % mBufSize;
        written  += remain;
    }

    if (written > 0 && mWritePos == mReadPos) {
        mIsFull = true;
    }

    return written;
}