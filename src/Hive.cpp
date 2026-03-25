#include "Hive.hpp"

#include <iostream>
#include <print>
#include <thread>

Hive::Hive(const std::string &auth_token, const Endpoint &endpoint)
    : m_auth_token(auth_token), m_endpoint(endpoint)
{
}

Hive::~Hive()
{
    m_io_context.stop();
    std::println("Hive is shutting down...");
}

void
Hive::run()
{
    using asio::ip::tcp;

    tcp::acceptor acceptor(m_io_context,
                           tcp::endpoint(tcp::v4(), m_endpoint.port));

    std::println("Hive is running on {}:{}", m_endpoint.host, m_endpoint.port);

    while (true)
    {
        tcp::socket socket(m_io_context);
        acceptor.accept(socket);

        // Handle the connection in a separate thread
        std::thread([this, socket = std::move(socket)]() mutable
        {
            try
            {
                handle_connection(std::move(socket));
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error handling connection: " << e.what()
                          << std::endl;
            }
        }).detach();
    }
    // m_io_context = asio::io_context();
}

void
Hive::handle_connection(asio::ip::tcp::socket socket)
{
    // Placeholder for handling the connection
    // You can read/write to the socket here
    std::println("Handling connection from {}:{}",
                 socket.remote_endpoint().address().to_string(),
                 socket.remote_endpoint().port());

    // For demonstration, we'll just close the connection immediately
    socket.close();
}
