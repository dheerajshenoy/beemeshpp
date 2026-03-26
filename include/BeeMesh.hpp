#pragma once

#include "Endpoint.hpp"
#include "argparse.hpp"

#include <asio.hpp>

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
    asio::io_context m_io_context;
};
