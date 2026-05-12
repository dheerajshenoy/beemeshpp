#pragma once

#include "MessageType.hpp"

#include <asio.hpp>
#include <nlohmann/json.hpp>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    explicit Connection(asio::ip::tcp::socket socket);
    ~Connection();

    MessageType get_request_type();
    std::string get_data();
    void write(const std::string &data);
    void read();

private:
    asio::ip::tcp::socket m_socket;
    nlohmann::json m_json_data;
    MessageType m_request_type;
};
