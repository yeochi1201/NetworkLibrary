#include "RecvBuffer.h"


RecvBuffer::RecvBuffer(size_t bufSize)
    : mBufSize(bufSize), mIsFull(false), mIsOpen(false), mReadPos(0), mWritePos(0)
{
}

RecvBuffer::~RecvBuffer() = default;

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
