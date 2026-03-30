#pragma once

#include "Endpoint.hpp"
#include "argparse.hpp"

#include <asio.hpp>
#include <nlohmann/json.hpp>

using tcp = asio::ip::tcp;

class BeeMesh
{
public:
    BeeMesh();
    ~BeeMesh();

    void start_hive_mode(const argparse::ArgumentParser &argparser);
    void start_bee_mode(const argparse::ArgumentParser &argparser);
    void start_monitor_mode(const argparse::ArgumentParser &argparser);
    void start_launch_mode(const argparse::ArgumentParser &argparser);

private:
    void send_device_registration(std::shared_ptr<tcp::socket> socket);
    void do_client_read_loop(std::shared_ptr<tcp::socket> socket,
                             std::shared_ptr<asio::streambuf> buffer);

private:
    asio::io_context m_io_context;
};
