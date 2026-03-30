#include "BeeMesh.hpp"

#include "Hive.hpp"
#include "Utils.hpp"

#include <iostream>
#include <print>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
#endif

const char *BANNER_STR = R"(
______            ___  ___          _
| ___ \           |  \/  |         | |
| |_/ / ___  ___  | .  . | ___  ___| |__
| ___ \/ _ \/ _ \ | |\/| |/ _ \/ __| '_ \
| |_/ /  __/  __/ | |  | |  __/\__ \ | | |
\____/ \___|\___| \_|  |_/\___||___/_| |_|
    _
   /_/_      .'''.      .''.   ..   .
=O(_)))) ...'     `.  .'    '.'  '.'
   \_\              ``

BeeMesh - Distributed Volunteer Computing Framework
)";

BeeMesh::BeeMesh() {}

BeeMesh::~BeeMesh()
{
    m_io_context.stop();
    std::println("BeeMesh is shutting down...");
}

void
BeeMesh::start_hive_mode(const argparse::ArgumentParser &argparser)
{
    std::string auth_token = argparser.get<std::string>("--auth-token");
    std::string host       = argparser.get<std::string>("--host");
    Port port              = argparser.get<Port>("--port");

    auto hive = std::make_shared<Hive>(m_io_context, auth_token,
                                       Endpoint{host, port});
    hive->run();

    m_io_context.run();
}

void
BeeMesh::start_bee_mode(const argparse::ArgumentParser &argparser)
{
    auto host = argparser.get<std::string>("--host");
    auto port = argparser.get<Port>("--port");

    auto socket   = std::make_shared<tcp::socket>(m_io_context);
    auto resolver = std::make_shared<tcp::resolver>(m_io_context);

    // Resolve endpoint asynchronously
    resolver->async_resolve(host, std::to_string(port),
                            [this, socket](const std::error_code &ec,
                                           tcp::resolver::results_type results)
    {
        if (ec)
        {
            std::cerr << "Resolve failed: " << ec.message() << "\n";
            return;
        }

        // Connect asynchronously
        asio::async_connect(
            *socket, results,
            [this, socket](const std::error_code &ec, const tcp::endpoint &)
        {
            if (ec)
            {
                std::cerr << "Connect failed: " << ec.message() << "\n";
                return;
            }

            send_device_registration(socket);

            auto buffer = std::make_shared<asio::streambuf>();
            do_client_read_loop(socket, buffer);
        });
    });

    m_io_context.run();
}

void
BeeMesh::send_device_registration(std::shared_ptr<tcp::socket> socket)
{
    // Build JSON message
    HostInfo hinfo = Utils::get_host_info();
    nlohmann::json json;
    json["msg_type"] = "bee";
    json["payload"]  = hinfo;
    std::string msg  = json.dump() + "\n"; // delimiter

    // Async write
    asio::async_write(
        *socket, asio::buffer(msg),
        [socket](const std::error_code &ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            std::cerr << "Async write failed: " << ec.message() << "\n";
        }
        else
        {
            std::cout << "Sent " << bytes_transferred << " bytes\n";
        }
    });
}

void
BeeMesh::do_client_read_loop(std::shared_ptr<tcp::socket> socket,
                             std::shared_ptr<asio::streambuf> buffer)
{
    asio::async_read_until(
        *socket, *buffer, "\n",
        [this, socket, buffer](const std::error_code &ec, std::size_t)
    {
        if (!ec)
        {
        }
        else
        {
            std::println("Disconnected from Hive: {}", ec.message());
        }
    });
}

void
BeeMesh::start_launch_mode(const argparse::ArgumentParser &argparser)
{
    auto host    = argparser.get<std::string>("--host");
    auto port    = argparser.get<Port>("--port");
    auto payload = argparser.get<std::string>("--payload");

    // 1. Move socket to the heap so it outlives this function
    auto socket   = std::make_shared<tcp::socket>(m_io_context);
    auto resolver = std::make_shared<tcp::resolver>(m_io_context);

    // 2. Resolve and Connect
    resolver->async_resolve(
        host, std::to_string(port),
        [this, socket, payload](const std::error_code &ec,
                                tcp::resolver::results_type results)
    {
        if (ec)
            return;

        asio::async_connect(*socket, results,
                            [this, socket, payload](const std::error_code &ec,
                                                    const tcp::endpoint &)
        {
            if (ec)
            {
                std::cerr << "Connect failed: " << ec.message() << "\n";
                return;
            }

            // 3. Send the registration message
            nlohmann::json json;
            json["msg_type"] = "launch";
            json["payload"]  = payload;
            std::string msg  = json.dump() + "\n";

            auto message_ptr = std::make_shared<std::string>(std::move(msg));

            asio::async_write(*socket, asio::buffer(*message_ptr),
                              [this, socket, message_ptr](
                                  const std::error_code &ec, std::size_t)
            {
                if (!ec)
                {
                    std::println("Successfully registered with Hive. Waiting "
                                 "for commands...");

                    // 4. START THE LOOP: Wait for commands from the Hive
                    auto read_buffer = std::make_shared<asio::streambuf>();
                    do_client_read_loop(socket, read_buffer);
                }
            });
        });
    });

    // 5. This blocks until the socket closes or the program is stopped
    m_io_context.run();
}

void
BeeMesh::start_monitor_mode(const argparse::ArgumentParser &argparser)
{
}
