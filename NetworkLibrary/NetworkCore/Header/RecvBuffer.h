    #ifndef RECV_BUFFER_H
    #define RECV_BUFFER_H

    #include <sys/types.h>
    #include <memory>
    #include <cstdint>
    #include "RingBuffer.h"

    enum eRecvBufferError{
        RecvBuf_Ok = 0,
        RecvBuf_NotOpen,
        RecvBuf_InvalidArgs,
        RecvBuf_Overflow,
        RecvBuf_Underflow,
        RecvBuf_InternalError,
    };

    class RecvBuffer{
    public:
        explicit RecvBuffer(size_t bufSize);
        ~RecvBuffer();

        RecvBuffer(const RecvBuffer&) = delete;
        RecvBuffer& operator=(const RecvBuffer&) = delete;

        RecvBuffer(RecvBuffer&& other) noexcept;
        RecvBuffer& operator=(RecvBuffer&& other) noexcept;

        eRecvBufferError Open();
        eRecvBufferError Reset();
        void Close();
        eRecvBufferError Read(void* dst, size_t len, size_t& outRead);
        eRecvBufferError Peek(void* dst, size_t len, size_t& outPeek);
        eRecvBufferError Consume(size_t len);
        eRecvBufferError Write(const void* src, size_t len, size_t& outWrite);
        
        size_t BufSize() const noexcept;
        size_t WriteSpace() const noexcept;
        size_t FreeSpace() const noexcept;

        bool IsOpen() const;
        bool IsEmpty() const;
        bool IsFull() const;        

    private:
        std::unique_ptr<RingBuffer> mRingBuffer;
        bool mIsOpen{false};
    };

    #endif