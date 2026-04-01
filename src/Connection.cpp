#include "Connection.hpp"

Connection::Connection(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket))
{
}

Connection::~Connection()
{
    m_socket.close();
}

RequestType
Connection::get_request_type()
{
    if (m_json_data.empty())
    {
        asio::streambuf buffer;
        asio::read_until(m_socket, buffer, "\n");
        std::istream is(&buffer);
        std::string data;
        std::getline(is, data);
        m_json_data    = nlohmann::json::parse(data);
        m_request_type = get_request_type_from_string();
    }
    return m_request_type;
}

std::string
Connection::get_data()
{
    if (m_json_data.empty())
    {
        asio::streambuf buffer;
        asio::read_until(m_socket, buffer, "\n");
        std::istream is(&buffer);
        std::string data;
        std::getline(is, data);
        m_json_data = nlohmann::json::parse(data);
    }
    return m_json_data["data"].get<std::string>();
}

RequestType
Connection::get_request_type_from_string()
{
    std::string type_str = m_json_data["type"].get<std::string>();

    if (type_str == "bee")
        return RequestType::Bee;

    else if (type_str == "monitor")
        return RequestType::Monitor;

    else if (type_str == "launch")
        return RequestType::Launch;

    else if (type_str == "broadcast")
        return RequestType::Broadcast;

    else
        throw std::runtime_error("Unknown request type: " + type_str);
}
