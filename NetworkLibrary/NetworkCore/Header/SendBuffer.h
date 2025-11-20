#ifndef SENDBUFFER_H
#define SENDBUFFER_H

#include <sys/types.h>
#include <memory>
#include <cstdint>

enum eSendBufferError{
    SENDBUFFER_ERROR_NONE = 0,
    SENDBUFFER_ERROR_OVERFLOW,
    SENDBUFFER_ERROR_UNDERFLOW,
    SENDBUFFER_ERROR_NOT_OPEN,
    SENDBUFFER_ERROR_INVALID_ARGS
};

class SendBuffer
{
public:
    explicit SendBuffer(size_t bufSize);
    ~SendBuffer();

    SendBuffer(const SendBuffer&) = delete;
    SendBuffer& operator=(const SendBuffer) = delete;

    SendBuffer(SendBuffer&& other) noexcept;
    SendBuffer& operator=(SendBuffer&& other) noexcept;

    eSendBufferError Open();
    eSendBufferError Reset();
    void Close();
    eSendBufferError Write(const void* src, size_t len, size_t& outWritten);
    eSendBufferError Read(void* dst, size_t len, size_t& outRead);
    eSendBufferError Peek(void* dst, size_t len, size_t& outPeek);
    eSendBufferError Consume(size_t len);

    size_t BufSize() const noexcept;
    size_t ReadableBytes() const noexcept;
    size_t WritableBytes() const noexcept;

    bool IsOpen() const;
    bool IsEmpty() const;
    bool IsFull() const;
private:
    std::unique_ptr<std::uint8_t[]> mBuf;
    bool mIsFull{false};
    bool mIsOpen{false};
    size_t mReadPos;
    size_t mWritePos;
    size_t mBufSize;
};

#endif