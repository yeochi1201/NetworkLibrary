#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <sys/types.h>
#include <memory>
#include <cstdint>

class RingBuffer{
public:
    explicit RingBuffer(size_t bufSize);
    ~RingBuffer();

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer) = delete;

    RingBuffer(RingBuffer&& other) noexcept;
    RingBuffer& operator=(RingBuffer&& other) noexcept;

    void Reset();
    void Close();
    std::size_t Read(void* dst, size_t len, size_t& outRead);
    std::size_t Peek(void* dst, size_t len, size_t& outPeek);
    void Consume(size_t len);
    std::size_t Write(const void* src, size_t len, size_t& outWrite);
    
    size_t BufSize() const noexcept;
    size_t WriteSpace() const noexcept;
    size_t FreeSpace() const noexcept;

    bool IsEmpty() const;
    bool IsFull() const;        

private:
    std::unique_ptr<std::uint8_t[]> mBuf;
    bool mIsFull{false};
    size_t mReadPos;
    size_t mWritePos;
    size_t mBufSize;
};

#endif