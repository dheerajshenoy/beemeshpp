#pragma once

#include "Job.hpp"
#include "Protocol.hpp"

#include <asio.hpp>
#include <cstdint>
#include <string>

using BeeId = uint64_t;
using asio::ip::tcp;

class Hive;

class Bee : public std::enable_shared_from_this<Bee>
{
public:
    Bee(tcp::socket socket, BeeId id, Hive *hive);
    ~Bee();

    // Prevent copying to protect the socket resource
    Bee(const Bee &)            = delete;
    Bee &operator=(const Bee &) = delete;

    inline BeeId id() const
    {
        return m_id;
    }

    inline const std::string &name() const
    {
        return m_name;
    }

    void run();

private:
    void do_send_status();

private:
    BeeId m_id;
    std::string m_name;
    asio::ip::tcp::socket m_socket;
    Hive *m_hive;
};
