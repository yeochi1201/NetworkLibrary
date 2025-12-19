#ifndef MESSAGE_FRAMER
#define MESSAGE_FRAMER

#include <cstdint>
#include <vector>

struct Frame{
    std::vector<std::uint8_t> payload;
};

enum class eFrameError{
    Framer_Ok,
    Framer_NeedMore,
    Framer_NotOpen,
    Framer_InvalidArgs,
    Framer_Overflow,
    Framer_BufferError
};

class RecvBuffer;

class MessageFramer{
public:
    static constexpr std::uint32_t kMaxPayload = 1u * 1024u * 1024u;
    static constexpr std::size_t kHeaderSize = 4;

    static eFrameError PopFrame(RecvBuffer& rb, Frame& out);
    static eFrameError Encode(const void* data, std::size_t len, std::vector<std::uint8_t> &out);
private:
    static std::uint32_t ReadU32BE(const std::uint8_t* p);
    static void WriteU32BE(std::uint8_t* p, std::uint32_t v);
};

#endif