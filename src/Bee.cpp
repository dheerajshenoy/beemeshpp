#include "Bee.hpp"

#include "Connection.hpp"

#include <iostream>
#include <print>

Bee::Bee(const std::string &auth_token, const std::string &host,
         const std::string &port)
    : m_auth_token(auth_token), m_host(host), m_port(port),
      m_socket(m_io_context)
{
    init_connection();
}

Bee::~Bee()
{
    std::println("Bee [id: {}] is shutting down...", m_id);
}

void
Bee::init_connection()
{
    std::println("Bee is connecting to Hive at {}:{} with auth token '{}'",
                 m_host, m_port, m_auth_token);
    tcp::resolver resolver(m_io_context);
    asio::connect(m_socket, resolver.resolve(m_host, m_port));
}

// Inside Bee.cpp
void
Bee::run()
{
    nlohmann::json data = {{"type", "bee"}, {"data", "<device info>"}};
    asio::async_write(m_socket, asio::buffer(data.dump() + MSG_DELIMITER),
                      [this](const std::error_code &ec, std::size_t length)
    {
        if (ec)
        {
            std::cerr << "Error sending data to Hive: " << ec.message()
                      << std::endl;
            return;
        }
        std::println("Bee [id: {}] sent device info to Hive", m_id);
        read_from_hive();
    });
    m_io_context.run();
}

void
Bee::start_job()
{
    if (!m_job)
        return;

    m_status = Status::Running;
}

void
Bee::stop_job()
{
    if (!m_job)
        return;

    m_status = Status::Stopped;
}

void
Bee::resume_job()
{
    if (!m_job)
        return;

    m_status = Status::Running;
}

void
Bee::read_from_hive()
{

    asio::async_read_until(m_socket, asio::dynamic_buffer(m_buffer),
                           MSG_DELIMITER,
                           [this](const std::error_code &ec, std::size_t length)
    {
        if (ec)
        {
            std::cerr << "Error reading from Hive: " << ec.message() << "\n";
            return;
        }

        // Extract exactly one message worth of bytes
        std::string message(m_buffer.begin(), m_buffer.begin() + length);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + length);

        try
        {
            auto json = nlohmann::json::parse(message);
            // handle_message(json);
        }
        catch (const nlohmann::json::exception &e)
        {
            std::cerr << "Failed to parse message from Hive: " << e.what()
                      << "\n";
        }

        read_from_hive();
    });
}
