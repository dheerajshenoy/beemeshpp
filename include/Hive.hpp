#pragma once

#include "Bee.hpp"
#include "Endpoint.hpp"
#include "Job.hpp"

#include <asio.hpp>

class Hive
{
public:
    Hive(const std::string &auth_token, const Endpoint &endpoint);
    ~Hive();

    void run();

private:
    void handle_connection(asio::ip::tcp::socket socket);

private:
    std::string m_auth_token;
    Endpoint m_endpoint;
    asio::io_context m_io_context;
};
