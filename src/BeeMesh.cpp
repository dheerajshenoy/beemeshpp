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

// void
// BeeMesh::start_bee_mode(const argparse::ArgumentParser &argparser)
// {
//     auto host = argparser.get<std::string>("--host");
//     auto port = argparser.get<Port>("--port");

//     // Keep the socket alive using a shared_ptr
//     auto socket   = std::make_shared<asio::ip::tcp::socket>(m_io_context);
//     auto resolver = std::make_shared<asio::ip::tcp::resolver>(m_io_context);

//     try
//     {
//         asio::connect(*socket, resolver->resolve(host,
//         std::to_string(port)));
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Error in connection: " << e.what() << "\n";
//         return;
//     }

//     // Get the host info
//     nlohmann::json json;

//     HostInfo hinfo   = Utils::get_host_info();
//     json["msg_type"] = "bee";
//     json["payload"]  = hinfo;

//     asio::async_write(
//         *socket,
//         asio::buffer(json.dump() + "\n"), // Add newline as a message
//         delimiter
//                                           // for the Hive to read until
//         [socket](const std::error_code &ec, std::size_t bytes_transferred)
//     {
//         if (ec)
//         {
//             std::cerr << "Async write failed: " << ec.message() << "\n";
//         }
//         else
//         {
//             std::cout << "Sent " << bytes_transferred << " bytes\n";
//         }
//     });

//     m_io_context.run();
// }

void
BeeMesh::start_bee_mode(const argparse::ArgumentParser &argparser)
{
    auto host = argparser.get<std::string>("--host");
    auto port = argparser.get<Port>("--port");

    auto socket   = std::make_shared<asio::ip::tcp::socket>(m_io_context);
    auto resolver = std::make_shared<asio::ip::tcp::resolver>(m_io_context);

    // Resolve endpoint asynchronously
    resolver->async_resolve(
        host, std::to_string(port),
        [this, socket](const std::error_code &ec,
                       asio::ip::tcp::resolver::results_type results)
    {
        if (ec)
        {
            std::cerr << "Resolve failed: " << ec.message() << "\n";
            return;
        }

        // Connect asynchronously
        asio::async_connect(*socket, results,
                            [this, socket](const std::error_code &ec,
                                           const asio::ip::tcp::endpoint &)
        {
            if (ec)
            {
                std::cerr << "Connect failed: " << ec.message() << "\n";
                return;
            }

            // Build JSON message
            HostInfo hinfo = Utils::get_host_info();
            nlohmann::json json;
            json["msg_type"] = "bee";
            json["payload"]  = hinfo;
            std::string msg  = json.dump() + "\n"; // delimiter

            // Async write
            asio::async_write(*socket, asio::buffer(msg),
                              [socket](const std::error_code &ec,
                                       std::size_t bytes_transferred)
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
        });
    });

    m_io_context.run();
}

void
BeeMesh::start_launch_mode(const argparse::ArgumentParser &argparser)
{
    auto host    = argparser.get<std::string>("--host");
    auto port    = argparser.get<Port>("--port");
    auto payload = argparser.get<std::string>("--payload");

    asio::ip::tcp::socket socket(m_io_context);
    asio::ip::tcp::resolver resolver(m_io_context);

    try
    {
        asio::connect(socket, resolver.resolve(host, std::to_string(port)));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in connection: " << e.what();
    }
}

void
BeeMesh::start_monitor_mode(const argparse::ArgumentParser &argparser)
{
}
