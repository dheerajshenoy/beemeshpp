#pragma once

#include <asio.hpp>
#include <nlohmann/json.hpp>

enum class RequestType
{
    Bee = 0,
    Monitor,
    Launch,
    Broadcast
};

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(asio::ip::tcp::socket socket);
    ~Connection();

    RequestType get_request_type();
    std::string get_data();
    asio::ip::tcp::socket &socket()
    {
        return m_socket;
    }

private:
    RequestType get_request_type_from_string();

private:
    asio::ip::tcp::socket m_socket;
    nlohmann::json m_json_data;
    RequestType m_request_type;
};
