#ifndef MESSAGE_FRAMER
#define MESSAGE_FRAMER

#include <cstdint>
#include <vector>

struct Frame{
    std::vector<std::uint8_t> payload;
};

class RecvBuffer;

class MessageFramer{
public:
    static constexpr std::uint32_t maxPayload = 1u * 1024u * 1024u;
    static bool PopFrame(RecvBuffer& rb, Frame& out);
    static void Encode(const void* data, std::size_t len, std::vector<std::uint8_t> &out);
private:
    static std::uint32_t ReadU32BE(const std::uint8_t* p);
    static void WriteU32BE(std::uint8_t* p, std::uint32_t v);
};

#endif