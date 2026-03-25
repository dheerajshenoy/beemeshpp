#pragma once

#include <cstdint>
#include <string>

using BeeId = uint64_t;

class Bee
{
public:
    Bee(BeeId id, const std::string &name);
    ~Bee();

private:
    BeeId m_id;
    std::string m_name;
};
