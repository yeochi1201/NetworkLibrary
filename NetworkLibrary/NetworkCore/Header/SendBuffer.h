#ifndef SEND_BUFFER_H
#define SEND_BUFFER_H

#include <sys/types.h>
#include <memory>
#include <cstdint>
#include "RingBuffer.h"

enum eSendBufferError{
    SendBuf_Ok = 0,
    SendBuf_NotOpen,
    SendBuf_InvalidArgs,
    SendBuf_Overflow,
    SendBuf_Underflow,
    SendBuf_InternalError,
};

class SendBuffer{
public:
    explicit SendBuffer(size_t bufSize);
    ~SendBuffer();

    SendBuffer(const SendBuffer&) = delete;
    SendBuffer& operator=(const SendBuffer&) = delete;

    SendBuffer(SendBuffer&& other) noexcept;
    SendBuffer& operator=(SendBuffer&& other) noexcept;

    eSendBufferError Open();
    eSendBufferError Reset();
    void             Close();

    eSendBufferError Write(const void* src, size_t len, size_t& outWrite);
    eSendBufferError Read(void* dst, size_t len, size_t& outRead);
    eSendBufferError Peek(void* dst, size_t len, size_t& outPeek);
    eSendBufferError Consume(size_t len);

    size_t BufSize()    const noexcept;
    size_t WriteSpace() const noexcept; 
    size_t FreeSpace()  const noexcept;

    bool IsOpen()  const;
    bool IsEmpty() const;
    bool IsFull()  const;

private:
    std::unique_ptr<RingBuffer> mRingBuffer;
    bool                        mIsOpen{false};
};

#endif
