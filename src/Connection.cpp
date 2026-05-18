#include "Connection.hpp"

#include "Utils.hpp"

#include <iostream>

Connection::Connection(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket))
{
}

Connection::~Connection()
{
    m_socket.close();
}

void
Connection::write(const std::string &data)
{
    auto self = shared_from_this();
    asio::async_write(m_socket, asio::buffer(data),
                      [self](const std::error_code &ec, std::size_t)
    {
        if (ec)
            std::cerr << "Connection write error: " << ec.message() << "\n";
    });
}

void
Connection::read_async(
    std::function<void(MessageType, const nlohmann::json &)> on_message,
    std::function<void()> on_disconnect)
{
    auto self = shared_from_this();
    asio::async_read_until(
        m_socket, asio::dynamic_buffer(m_read_buffer), "\n",
        [this, self, on_message, on_disconnect](const std::error_code &ec,
                                                std::size_t length)
    {
        if (ec)
        {
            if (on_disconnect)
                on_disconnect();
            return;
        }

        std::string raw(m_read_buffer.begin(),
                        m_read_buffer.begin() + length);
        m_read_buffer.erase(0, length);

        try
        {
            auto json_msg = nlohmann::json::parse(raw);
            auto type     = Utils::message_type_from_string(
                json_msg.at("type").get<std::string>());
            on_message(type, json_msg.at("data"));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Connection parse error: " << e.what() << "\n";
        }

        read_async(on_message, on_disconnect);
    });
}
