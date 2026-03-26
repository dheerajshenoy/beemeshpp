#include "Bee.hpp"

#include "Hive.hpp"

#include <print>

Bee::Bee(asio::ip::tcp::socket socket, BeeId id, Job job, Hive *hive)
    : m_socket(std::move(socket)), m_id(id), m_hive(hive), m_job(job)
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
    start_job_execution();
    do_send_status();
}

void
Bee::start_job_execution()
{
}

void
Bee::do_send_status()
{
    auto self = shared_from_this();

    asio::async_write(
        m_socket, asio::buffer("HELLO from BEE " + std::to_string(m_id) + "\n"),
        [this, self](const std::error_code &ec, std::size_t /* length */)
    {
        if (ec)
        {
            // TODO: Error Handling
            return;
        }

        m_hive->broadcast_payload(BroadcastType::StatusUpdate, m_id,
                                  "HELLO from BEE " + std::to_string(m_id));

        std::println("Bee sent message to hive");
    });
}
