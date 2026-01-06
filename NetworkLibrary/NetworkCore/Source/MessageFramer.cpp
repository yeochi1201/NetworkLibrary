#include "MessageFramer.h"
#include "RecvBuffer.h"
#include <cstring>
#include <stdexcept>

std::uint32_t MessageFramer::ReadU32BE(const std::uint8_t* p){
    return  (static_cast<std::uint32_t>(p[0]) << 24) |
            (static_cast<std::uint32_t>(p[1]) << 16) |
            (static_cast<std::uint32_t>(p[2]) << 8) |
            (static_cast<std::uint32_t>(p[3]) << 0);
}

void MessageFramer::WriteU32BE(std::uint8_t* p, std::uint32_t v){
    p[0] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    p[2] = static_cast<std::uint8_t>((v >>  8) & 0xFF);
    p[3] = static_cast<std::uint8_t>((v >>  0) & 0xFF);
}

eFrameError MessageFramer::Encode(const void* data, std::size_t len, std::vector<std::uint8_t>& out){
    if(data == nullptr && len != 0) return eFrameError::Framer_InvalidArgs;
    if(len > kMaxPayload) return eFrameError::Framer_Overflow;
    out.resize(kHeaderSize + len);
    WriteU32BE(out.data(), static_cast<std::uint32_t>(len));
    if(len) std::memcpy(out.data() + kHeaderSize, data, len);
    return eFrameError::Framer_Ok;
}

eFrameError MessageFramer::PopFrame(RecvBuffer &rb, Frame &out){
    if(!rb.IsOpen()) return eFrameError::Framer_NotOpen;
    const std::size_t available = rb.WriteSpace();
    if(available < kHeaderSize) return eFrameError::Framer_NeedMore;

    std::uint8_t hdr[kHeaderSize]{};
    std::size_t outPeek = 0;
    {
        const eRecvBufferError err = rb.Peek(hdr, kHeaderSize, outPeek);
        if(err == RecvBuf_Underflow)    return eFrameError::Framer_NeedMore;
        if(err == RecvBuf_NotOpen)      return eFrameError::Framer_NotOpen;
        if(err != RecvBuf_Ok)           return eFrameError::Framer_BufferError;
        if(outPeek != kHeaderSize)      return eFrameError::Framer_NeedMore;
    }

    const std::uint32_t len = ReadU32BE(hdr);
    if(len > kMaxPayload) return eFrameError::Framer_Overflow;

    const std::size_t total = kHeaderSize + static_cast<std::size_t>(len);
    if(available < total) return eFrameError::Framer_NeedMore;

    {
        const eRecvBufferError err = rb.Consume(kHeaderSize);
        if(err == RecvBuf_Underflow)    return eFrameError::Framer_NeedMore;
        if(err == RecvBuf_NotOpen)      return eFrameError::Framer_NotOpen;
        if(err != RecvBuf_Ok)           return eFrameError::Framer_BufferError;
    }

    out.payload.resize(len);
    if(len > 0){
        std::size_t outRead = 0;
        const eRecvBufferError err = rb.Read(out.payload.data(), len, outRead);
        if(err == RecvBuf_Underflow)    return eFrameError::Framer_NeedMore;
        if(err == RecvBuf_NotOpen)      return eFrameError::Framer_NotOpen;
        if(err != RecvBuf_Ok)           return eFrameError::Framer_BufferError;

        if(outRead != len) return eFrameError::Framer_BufferError;
    }
    return eFrameError::Framer_Ok;
}