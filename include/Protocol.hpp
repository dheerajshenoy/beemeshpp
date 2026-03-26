#pragma once

#include <cstdint>

enum class MessageType : uint8_t
{
    Handshake = 0,
    Launch, // This is for your Job submissions
    Status, // For querying job progress
    Result,
    Bye // Graceful disconnect
};

#pragma pack(push, 1) // Ensure no padding for network efficiency
struct BeeMessageHeader
{
    MessageType type;
    // Compiler might add 7 bytes of "garbage" here to align the next 8-byte
    // value!
    uint64_t payload_size;
};
#pragma pack(pop)
