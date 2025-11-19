#include "RecvBuffer.h"

#include <cstring>

RecvBuffer::RecvBuffer(size_t bufSize)
    : mBufSize(bufSize), mIsFull(false), mIsOpen(false), mReadPos(0), mWritePos(0)
{
}

RecvBuffer::~RecvBuffer()
{
    Close();
}

RecvBuffer::RecvBuffer(RecvBuffer&& other) noexcept
    : mBufSize(other.mBufSize), mBuf(std::move(other.mBuf)),mIsFull(other.mIsFull), mIsOpen(other.mIsOpen), mReadPos(other.mReadPos), mWritePos(other.mWritePos)
{
    other.mIsFull = false;
    other.mIsOpen = false;
    other.mReadPos = 0;
    other.mWritePos = 0;
    other.mBufSize = 0;
}

RecvBuffer& RecvBuffer::operator=(RecvBuffer&& other) noexcept
{
    if (this != &other) {
        mBuf      = std::move(other.mBuf);
        mIsFull   = other.mIsFull;
        mIsOpen   = other.mIsOpen;
        mReadPos  = other.mReadPos;
        mWritePos = other.mWritePos;
        mBufSize  = other.mBufSize;

        other.mIsFull   = false;
        other.mIsOpen   = false;
        other.mReadPos  = 0;
        other.mWritePos = 0;
        other.mBufSize  = 0;
    }
    return *this;
}

eRecvBufferError RecvBuffer::Open()
{
    if(mIsOpen || mBufSize == 0)
        return RecvBuf_InternalError;
    mReadPos  = 0;
    mWritePos = 0;
    mIsFull   = false;
    mIsOpen   = true;
    mBuf = std::make_unique<std::uint8_t[]>(mBufSize);

    return RecvBuf_Ok;
}

eRecvBufferError RecvBuffer::Reset()
{
    if(!mIsOpen)
        return RecvBuf_NotOpen;
    
    mReadPos  = 0;
    mWritePos = 0;
    mIsFull   = false;

    return RecvBuf_Ok;
}

void RecvBuffer::Close()
{
    mIsOpen = false;
    mReadPos = 0;
    mWritePos = 0;
    mIsFull = false;
    mBuf.reset();
}

size_t RecvBuffer::BufSize() const noexcept
{
    return mBufSize;
}
size_t RecvBuffer::WriteSpace() const noexcept
{
    if(!mIsOpen)
        return 0;

    if(mIsFull)
        return 0;

    if(mWritePos >= mReadPos)
        return mBufSize - (mWritePos - mReadPos);
    else
        return mReadPos - mWritePos;
}


size_t RecvBuffer::FreeSpace() const noexcept
{
    if(!mIsOpen)
        return 0;

    if(mIsFull)
        return 0;

    return mBufSize - (WriteSpace());
}

bool RecvBuffer::IsOpen() const
{
    return mIsOpen;
}

bool RecvBuffer::IsEmpty() const
{
    return mIsOpen ? (!mIsFull && (mReadPos == mWritePos)) : true;
}

bool RecvBuffer::IsFull() const
{
    return mIsOpen && mIsFull;
}

eRecvBufferError RecvBuffer::Read(void* dst, size_t len, size_t& outRead)
{
    outRead = 0;
    if(!mIsOpen)
        return RecvBuf_NotOpen;
    if(dst == nullptr || len == 0)
        return RecvBuf_InvalidArgs;

    size_t availableSpace = WriteSpace();
    if(availableSpace == 0)
        return RecvBuf_Underflow;

    size_t toRead = (len < availableSpace) ? len : availableSpace;
    uint8_t* out = static_cast<uint8_t*>(dst);

    size_t firstChunk = toRead;
    size_t untilEnd = mBufSize - mReadPos;
    if(firstChunk > untilEnd)
        firstChunk = untilEnd;
    
    std::memcpy(out, mBuf.get() + mReadPos, firstChunk);
    mReadPos = (mReadPos + firstChunk) % mBufSize;
    outRead += firstChunk;

    size_t remain = toRead - firstChunk;
    if(remain > 0) {
        std::memcpy(out + firstChunk, mBuf.get() + mReadPos, remain);
        mReadPos = (mReadPos + remain) % mBufSize;
        outRead += remain;
    }

    if(outRead > 0)
        mIsFull = false;
    return RecvBuf_Ok;

}
eRecvBufferError RecvBuffer::Peek(void* dst, size_t len, size_t& outPeek)
{
    outPeek = 0;
    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }
    if (dst == nullptr || len == 0) {
        return RecvBuf_InvalidArgs;
    }

    size_t available = WriteSpace();
    if (available == 0) {
        return RecvBuf_Underflow;
    }

    size_t toPeek = (len < available) ? len : available;

    uint8_t* out = static_cast<uint8_t*>(dst);

    size_t readPos = mReadPos;

    size_t firstChunk = toPeek;
    size_t untilEnd   = mBufSize - readPos;
    if (firstChunk > untilEnd) {
        firstChunk = untilEnd;
    }

    std::memcpy(out, mBuf.get() + readPos, firstChunk);
    readPos = (readPos + firstChunk) % mBufSize;
    outPeek += firstChunk;

    size_t remain = toPeek - firstChunk;
    if (remain > 0) {
        std::memcpy(out + firstChunk, mBuf.get() + readPos, remain);
        readPos = (readPos + remain) % mBufSize;
        outPeek += remain;
    }

    return RecvBuf_Ok;
}
eRecvBufferError RecvBuffer::Consume(size_t len)
{
    if(!mIsOpen)
        return RecvBuf_NotOpen;
    
    size_t availableSpace = WriteSpace();
    if(len > availableSpace)
        return RecvBuf_Underflow;
    
}
eRecvBufferError RecvBuffer::Write(const void* src, size_t len, size_t& outWrite)
{
    outWrite = 0;
    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }
    if (src == nullptr || len == 0) {
        return RecvBuf_InvalidArgs;
    }
    size_t freeSpace = FreeSpace();
    if (src == nullptr || len == 0) {
        return RecvBuf_InvalidArgs;
    }

    size_t toWrite = len;
    auto* in = static_cast<const uint8_t*>(src);

    size_t firstChunk = toWrite;
    size_t untilEnd = mBufSize - mWritePos;

    if(firstChunk > untilEnd)
        firstChunk = untilEnd;

    std::memcpy(mBuf.get() + mWritePos, in, firstChunk);
    mWritePos = (mWritePos + firstChunk) % mBufSize;
    outWrite += firstChunk;

    size_t remain = toWrite - firstChunk;
    if(remain > 0){
        std::memcpy(mBuf.get() + mWritePos, in + firstChunk, remain);
        mWritePos = (mWritePos + remain) % mBufSize;
        outWrite += remain;
    }

    if(mWritePos == mReadPos)
        mIsFull = true;

    return RecvBuf_Ok;
}