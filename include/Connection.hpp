#pragma once

#include "MessageType.hpp"

#include <asio.hpp>
#include <functional>
#include <nlohmann/json.hpp>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    explicit Connection(asio::ip::tcp::socket socket);
    ~Connection();

    void write(const std::string &data);
    void read_async(
        std::function<void(MessageType, const nlohmann::json &)> on_message,
        std::function<void()> on_disconnect = {});

private:
    asio::ip::tcp::socket m_socket;
    std::string m_read_buffer;
};
