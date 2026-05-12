#include "Bee.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

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
    nlohmann::json data
        = {{"type", Utils::to_string(MessageType::BEE_REGISTRATION)},
           {"data", "<device info>"}};

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

        read_from_hive();
        handle_message(message);
    });
}

void
Bee::handle_message(const std::string &message)
{
    try
    {
        const auto json_msg = nlohmann::json::parse(message);
        const auto type     = Utils::message_type_from_string(
            json_msg.at("type").get<std::string>());

        switch (type)
        {
            case MessageType::BEE_ID_ASSIGNMENT:
                m_id = std::stoi(
                    json_msg.at("data").at("bee_id").get<std::string>());
                std::println("Bee assigned id: {}", m_id);
                break;

            case MessageType::JOB_ASSIGNMENT:
                // handle job
                break;

            case MessageType::STATUS_UPDATE:
                // handle status
                break;

            case MessageType::STATUS_CHECK:
                std::println("Received status check from Hive");
                break;

            case MessageType::ERROR_REPORT:
                std::cerr << "Hive error: "
                          << json_msg.at("data").get<std::string>() << "\n";
                break;

            default:
                std::cerr << "Unexpected message type from Hive: "
                          << json_msg.at("type") << "\n";
                break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error handling message: " << e.what() << "\n";
    }
}
