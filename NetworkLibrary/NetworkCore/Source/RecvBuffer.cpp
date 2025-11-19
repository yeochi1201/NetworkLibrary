#include "RecvBuffer.h"


RecvBuffer::RecvBuffer(size_t bufSize)
    : mBufSize(bufSize), mBuf(std::make_unique<std::uint8_t[]>(mBufSize)), mIsFull(false), mIsOpen(false), mReadPos(0), mWritePos(0)
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