#include "Hive.hpp"

#include "Connection.hpp"

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <print>

Hive::Hive(const std::string &auth_token, const std::string &host,
           const std::string &port)
    : m_acceptor(m_io_context), m_auth_token(auth_token), m_host(host),
      m_port(port)
{
    init_connection();
}

Hive::~Hive()
{
    m_io_context.stop();
    std::println("Hive is shutting down...");
}

void
Hive::init_connection()
{
    try
    {
        // Convert host string to ASIO address
        tcp::resolver resolver(m_io_context);
        auto results = resolver.resolve(m_host, m_port);

        tcp::endpoint ep = *results.begin(); // take the first resolved endpoint

        m_acceptor.open(ep.protocol());
        m_acceptor.set_option(tcp::acceptor::reuse_address(true));
        m_acceptor.bind(ep);
        m_acceptor.listen();

        std::println("Hive listening on {}:{}", m_host, m_port);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to start Hive acceptor: " << e.what();
        throw;
    }
}

void
Hive::run()
{
    do_accept();
    assign_jobs_to_bees();

    m_io_context.run();
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
    auto conn         = std::make_shared<Connection>(std::move(socket));
    auto request_type = conn->get_request_type();
    auto data         = conn->get_data();
    std::println("Connection type: {}, data: {}",
                 static_cast<int>(request_type), data);
    switch (request_type)
    {
        case RequestType::Bee:
            add_bee(std::move(conn->socket()), nlohmann::json::parse(data));
            break;
        case RequestType::Monitor:
        case RequestType::Launch:
        case RequestType::Broadcast:
            break;
    }
}

void
Hive::add_bee(asio::ip::tcp::socket socket, const nlohmann::json &bee_info)
{
}

void
Hive::add_job(const std::string &data)
{
    static JobId id   = 0;
    JobId next_job_id = id++;

    std::println("Hive received new job with ID: {}, PAYLOAD: {}", id, data);

    auto job = std::make_unique<Job>(id, data);
    {
        std::lock_guard<std::mutex> lock(m_jobs_mutex);
        m_pending_jobs_queue.push(job->id());
        m_all_jobs[next_job_id] = std::move(job);
    }
}

void
Hive::assign_jobs_to_bees()
{
    std::lock_guard<std::mutex> jobs_lock(m_jobs_mutex);
    std::lock_guard<std::mutex> bees_lock(m_bees_mutex);

    if (m_pending_jobs_queue.empty() || m_bees.empty())
        return;
}
