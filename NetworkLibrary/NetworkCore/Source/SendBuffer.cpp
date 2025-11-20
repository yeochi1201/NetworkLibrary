#include "SendBuffer.h"

SendBuffer::SendBuffer(size_t bufSize)
    : mRingBuffer(std::make_unique<RingBuffer>(bufSize))
    , mIsOpen(false)
{
}

SendBuffer::~SendBuffer()
{
    Close();
}

SendBuffer::SendBuffer(SendBuffer&& other) noexcept
    : mRingBuffer(std::move(other.mRingBuffer))
    , mIsOpen(other.mIsOpen)
{
    other.mIsOpen = false;
}

SendBuffer& SendBuffer::operator=(SendBuffer&& other) noexcept
{
    if (this != &other) {
        mRingBuffer = std::move(other.mRingBuffer);
        mIsOpen     = other.mIsOpen;

        other.mIsOpen = false;
    }
    return *this;
}

eSendBufferError SendBuffer::Open()
{
    if (mIsOpen || !mRingBuffer || mRingBuffer->BufSize() == 0) {
        return SendBuf_InternalError;
    }

    mRingBuffer->Reset();
    mIsOpen = true;
    return SendBuf_Ok;
}

eSendBufferError SendBuffer::Reset()
{
    if (!mIsOpen) {
        return SendBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return SendBuf_InternalError;
    }

    mRingBuffer->Reset();
    return SendBuf_Ok;
}

void SendBuffer::Close()
{
    if (!mRingBuffer) {
        mIsOpen = false;
        return;
    }

    mRingBuffer->Reset();
    mIsOpen = false;
}

eSendBufferError SendBuffer::Write(const void* src, size_t len, size_t& outWrite)
{
    outWrite = 0;

    if (!mIsOpen) {
        return SendBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return SendBuf_InternalError;
    }
    if (src == nullptr || len == 0) {
        return SendBuf_InvalidArgs;
    }

    const size_t freeSpace = mRingBuffer->FreeSpace();
    if (len > freeSpace) {
        return SendBuf_Overflow;
    }

    outWrite = mRingBuffer->Write(src, len);
    if (outWrite != len) {
        return SendBuf_InternalError;
    }

    return SendBuf_Ok;
}

eSendBufferError SendBuffer::Read(void* dst, size_t len, size_t& outRead)
{
    outRead = 0;

    if (!mIsOpen) {
        return SendBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return SendBuf_InternalError;
    }
    if (dst == nullptr || len == 0) {
        return SendBuf_InvalidArgs;
    }

    const size_t available = mRingBuffer->DataSpace();
    if (available == 0) {
        return SendBuf_Underflow;
    }

    outRead = mRingBuffer->Read(dst, len);
    return SendBuf_Ok;
}

eSendBufferError SendBuffer::Peek(void* dst, size_t len, size_t& outPeek)
{
    outPeek = 0;

    if (!mIsOpen) {
        return SendBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return SendBuf_InternalError;
    }
    if (dst == nullptr || len == 0) {
        return SendBuf_InvalidArgs;
    }

    const size_t available = mRingBuffer->DataSpace();
    if (available == 0) {
        return SendBuf_Underflow;
    }

    outPeek = mRingBuffer->Peek(dst, len);
    return SendBuf_Ok;
}

eSendBufferError SendBuffer::Consume(size_t len)
{
    if (!mIsOpen) {
        return SendBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return SendBuf_InternalError;
    }

    const size_t available = mRingBuffer->DataSpace();
    if (len > available) {
        return SendBuf_Underflow;
    }

    mRingBuffer->Consume(len);
    return SendBuf_Ok;
}

size_t SendBuffer::BufSize() const noexcept
{
    if (!mRingBuffer) {
        return 0;
    }
    return mRingBuffer->BufSize();
}

size_t SendBuffer::WriteSpace() const noexcept
{
    if (!mIsOpen || !mRingBuffer) {
        return 0;
    }
    return mRingBuffer->DataSpace();
}

size_t SendBuffer::FreeSpace() const noexcept
{
    if (!mIsOpen || !mRingBuffer) {
        return 0;
    }
    return mRingBuffer->FreeSpace();
}

bool SendBuffer::IsOpen() const
{
    return mIsOpen;
}

bool SendBuffer::IsEmpty() const
{
    if (!mIsOpen || !mRingBuffer) {
        return true;
    }
    return mRingBuffer->IsEmpty();
}

bool SendBuffer::IsFull() const
{
    if (!mIsOpen || !mRingBuffer) {
        return false;
    }
    return mRingBuffer->IsFull();
}
