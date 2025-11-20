#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(size_t bufSize)
    : mRingBuffer(std::make_unique<RingBuffer>(bufSize))
    , mIsOpen(false)
{
}

RecvBuffer::~RecvBuffer()
{
    Close();
}

RecvBuffer::RecvBuffer(RecvBuffer&& other) noexcept
    : mRingBuffer(std::move(other.mRingBuffer))
    , mIsOpen(other.mIsOpen)
{
    other.mIsOpen = false;
}

RecvBuffer& RecvBuffer::operator=(RecvBuffer&& other) noexcept
{
    if (this != &other) {
        mRingBuffer = std::move(other.mRingBuffer);
        mIsOpen     = other.mIsOpen;

        other.mIsOpen = false;
    }
    return *this;
}

eRecvBufferError RecvBuffer::Open()
{
    
    if (mIsOpen || !mRingBuffer || mRingBuffer->BufSize() == 0) {
        return RecvBuf_InternalError;
    }

    mRingBuffer->Reset();
    mIsOpen = true;
    return RecvBuf_Ok;
}

eRecvBufferError RecvBuffer::Reset()
{
    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }

    if (!mRingBuffer) {
        return RecvBuf_InternalError;
    }

    mRingBuffer->Reset();
    return RecvBuf_Ok;
}

void RecvBuffer::Close()
{
    if (!mRingBuffer) {
        mIsOpen = false;
        return;
    }

    mRingBuffer->Reset();
    mIsOpen = false;
}

eRecvBufferError RecvBuffer::Read(void* dst, size_t len, size_t& outRead)
{
    outRead = 0;

    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return RecvBuf_InternalError;
    }
    if (dst == nullptr || len == 0) {
        return RecvBuf_InvalidArgs;
    }

    const std::size_t available = mRingBuffer->DataSpace();
    if (available == 0) {
        return RecvBuf_Underflow;
    }

    outRead = mRingBuffer->Read(dst, len);
    // 부분 읽기는 허용 – 최대 len까지 읽고 OK 반환
    return RecvBuf_Ok;
}

eRecvBufferError RecvBuffer::Peek(void* dst, size_t len, size_t& outPeek)
{
    outPeek = 0;

    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return RecvBuf_InternalError;
    }
    if (dst == nullptr || len == 0) {
        return RecvBuf_InvalidArgs;
    }

    const std::size_t available = mRingBuffer->DataSpace();
    if (available == 0) {
        return RecvBuf_Underflow;
    }

    outPeek = mRingBuffer->Peek(dst, len);
    return RecvBuf_Ok;
}

eRecvBufferError RecvBuffer::Consume(size_t len)
{
    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return RecvBuf_InternalError;
    }

    const std::size_t available = mRingBuffer->DataSpace();
    if (len > available) {
        return RecvBuf_Underflow;
    }

    mRingBuffer->Consume(len);
    return RecvBuf_Ok;
}

eRecvBufferError RecvBuffer::Write(const void* src, size_t len, size_t& outWrite)
{
    outWrite = 0;

    if (!mIsOpen) {
        return RecvBuf_NotOpen;
    }
    if (!mRingBuffer) {
        return RecvBuf_InternalError;
    }
    if (src == nullptr || len == 0) {
        return RecvBuf_InvalidArgs;
    }

    const std::size_t freeSpace = mRingBuffer->FreeSpace();
    if (len > freeSpace) {
        return RecvBuf_Overflow;
    }

    outWrite = mRingBuffer->Write(src, len);
    if (outWrite != len) {
        return RecvBuf_InternalError;
    }

    return RecvBuf_Ok;
}

size_t RecvBuffer::BufSize() const noexcept
{
    if (!mRingBuffer) {
        return 0;
    }
    return mRingBuffer->BufSize();
}

size_t RecvBuffer::WriteSpace() const noexcept
{
    if (!mIsOpen || !mRingBuffer) {
        return 0;
    }
    return mRingBuffer->DataSpace();
}

size_t RecvBuffer::FreeSpace() const noexcept
{
    if (!mIsOpen || !mRingBuffer) {
        return 0;
    }
    return mRingBuffer->FreeSpace();
}

bool RecvBuffer::IsOpen() const
{
    return mIsOpen;
}

bool RecvBuffer::IsEmpty() const
{
    if (!mIsOpen || !mRingBuffer) {
        return true;
    }
    return mRingBuffer->IsEmpty();
}

bool RecvBuffer::IsFull() const
{
    if (!mIsOpen || !mRingBuffer) {
        return false;
    }
    return mRingBuffer->IsFull();
}
