#include "BeeMesh.hpp"

#include "Hive.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
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

    {
        nlohmann::json j;
        j["host-info"] = host_name;
    }
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
