#include "Bee.hpp"

#include "Hive.hpp"

#include <iostream>
#include <print>

Bee::Bee(asio::ip::tcp::socket socket, BeeId id, Hive *hive)
    : m_socket(std::move(socket)), m_id(id), m_hive(hive)
{
    m_name = "Bee-" + std::to_string(m_id);
}

Bee::~Bee()
{
    std::println("Bee (id: {}, name: {}) is shutting down...", m_id, m_name);
}

// Inside Bee.cpp
void
Bee::run()
{

    asio::ip::tcp::socket::keep_alive option(true);
    m_socket.set_option(option);

    do_send_status();
}

void
Bee::start_job_execution()
{
    if (!m_job)
        return;

    m_status = Status::Running;
}

void
Bee::stop_job_execution()
{
    if (!m_job)
        return;

    m_status = Status::Stopped;
}

void
Bee::resume_job_execution()
{
    if (!m_job)
        return;

    m_status = Status::Running;
}

void
Bee::do_send_status()
{
    auto self       = shared_from_this();
    std::string msg = "HELLO from BEE " + std::to_string(m_id) + "\n";
    asio::async_write(
        m_socket, asio::buffer(msg),
        [this, self](const std::error_code &ec, std::size_t /* length */)
    {
        if (ec)
        {
            std::cerr << "Failed to send status update: " << ec.message()
                      << "\n";
            return;
        }

        m_hive->broadcast_payload(BroadcastType::StatusUpdate, m_id,
                                  "HELLO from BEE " + std::to_string(m_id));

        std::println("Bee sent message to hive");
    });
}

void
Bee::do_read()
{
    auto self = shared_from_this();

    // asio::async_read_until(
    //     m_socket, m_buffer, "\n",
    //     [this, self](const std::error_code &ec, std::size_t /* length */)
    // {
    //     if (ec)
    //     {
    //         std::cerr << "Failed to read from socket: " << ec.message() <<
    //         "\n"; return;
    //     }
    // });
}
