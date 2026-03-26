#include "Hive.hpp"

#include "Protocol.hpp"

#include <iostream>
#include <memory>
#include <print>

Hive::Hive(asio::io_context &io_context, const std::string &auth_token,
           const Endpoint &endpoint)
    : m_auth_token(auth_token), m_endpoint(endpoint), m_io_context(io_context),
      m_acceptor(io_context)
{
    try
    {
        // Convert host string to ASIO address
        tcp::resolver resolver(io_context);
        auto results = resolver.resolve(m_endpoint.host,
                                        std::to_string(m_endpoint.port));

        tcp::endpoint ep = *results.begin(); // take the first resolved endpoint

        m_acceptor.open(ep.protocol());
        m_acceptor.set_option(tcp::acceptor::reuse_address(true));
        m_acceptor.bind(ep);
        m_acceptor.listen();

        std::println("Hive listening on {}:{}", m_endpoint.host,
                     m_endpoint.port);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to start Hive acceptor: " << e.what();
        throw;
    }
}

Hive::~Hive()
{
    std::println("Hive is shutting down...");
}

void
Hive::run()
{
    do_accept();
}

void
Hive::do_accept()
{
    using asio::ip::tcp;

    m_acceptor.async_accept(
        [this](const std::error_code &ec, tcp::socket socket)
    {
        if (ec)
        {
            // TODO: Error handling
        }
        else
        {
            handle_connection(std::move(socket));
        }

        do_accept();
    });
}

void
Hive::handle_connection(asio::ip::tcp::socket socket)
{
    try
    {
        std::string buffer;
        std::size_t read_bytes;

        asio::async_read(socket, asio::buffer(buffer),
                         [&](const std::error_code &ec, std::size_t len)
        {
            if (ec)
            {
                // TODO: Error handling
            }
            else
            {
            }
        });
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error reading from connection: " << e.what() << "\n";
    }
}

void
Hive::add_bee(asio::ip::tcp::socket socket)
{
    static BeeId bee_id = 0;
    BeeId next_bee_id   = bee_id++;
    std::make_shared<Bee>(std::move(socket), next_bee_id, this)->run();
}

void
Hive::add_job(std::unique_ptr<Job> job)
{
    std::lock_guard<std::mutex> lock(m_jobs_mutex);
    m_pending_jobs_queue.push(job->id());
    m_all_jobs.push_back(std::move(job));
}
