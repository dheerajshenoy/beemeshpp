#include "Connection.hpp"

#include "Utils.hpp"

Connection::Connection(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket))
{
}

Connection::~Connection()
{
    m_socket.close();
}

MessageType
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
        m_request_type = Utils::message_type_from_string(
            m_json_data["type"].get<std::string>());
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

void
Connection::write(const std::string &data)
{
    asio::async_write(m_socket, asio::buffer(data),
                      [this](const std::error_code &ec, std::size_t /*length*/)
    {
        if (ec)
        {
            std::cerr << "Error writing to connection: " << ec.message()
                      << "\n";
        }
    });
}
