#pragma once

#include <argparse.hpp>
#include <asio.hpp>
#include <cstdint>

class BeeMesh
{
public:
    BeeMesh(const argparse::ArgumentParser &argparser);
    ~BeeMesh();

    enum class Mode
    {
        None = 0,
        Monitor,
        Launch,
        Hive,
        Bee
    };

private:
    void run();
    void parse_args(const argparse::ArgumentParser &argparser);
    void start_hive_mode();
    void start_bee_mode();
    void start_monitor_mode();
    void start_launch_mode();

private:
    Mode m_mode = Mode::None;

    // ASIO components for network communication
    asio::io_context m_io_context;
    asio::ip::tcp::acceptor m_tcp_acceptor{m_io_context};

    // Default values for network configuration
    uint16_t m_port{8080};
    std::string m_host{"localhost"};
    std::string m_auth_token;
};
