#pragma once

#include <cstdint>
#include <string>

using Port = std::uint16_t;

struct Endpoint
{
    std::string host;
    Port port;
};
