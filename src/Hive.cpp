#include "Hive.hpp"

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
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
    assign_jobs_to_bees();
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
    struct Context
    {
        asio::ip::tcp::socket sock;
        asio::streambuf buffer;
        Context(asio::ip::tcp::socket s) : sock(std::move(s)) {}
    };

    auto ctx = std::make_shared<Context>(std::move(socket));

    asio::async_read_until(ctx->sock, ctx->buffer, "\n",
                           [this, ctx](const std::error_code &ec, std::size_t)
    {
        if (!ec)
        {
            std::istream is(&ctx->buffer);
            std::string line;
            std::getline(is, line);

            auto json = nlohmann::json::parse(line);
            if (json["msg_type"] == "bee")
            {
                // Cleanly move the socket out of the context and into the Bee
                add_bee(std::move(ctx->sock), json["payload"]);
            }

            else if (json["msg_type"] == "launch")
            {
                add_job(json["payload"].dump());
            }
        }
    });
}

void
Hive::add_bee(asio::ip::tcp::socket socket, const nlohmann::json &bee_info)
{
    static BeeId bee_id = 0;
    BeeId next_bee_id   = bee_id++;

    auto bee = std::make_shared<Bee>(std::move(socket), next_bee_id, this);

    {
        std::lock_guard<std::mutex> lock(m_bees_mutex);
        m_bees.push_back(bee);
    }
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
Hive::broadcast_payload(BroadcastType type, BeeId sender_id,
                        const std::string &payload)
{
    switch (type)
    {
        case BroadcastType::StatusUpdate:
        {
            std::println("Hive broadcasting payload from Bee {}: {}", sender_id,
                         payload);
        }
        break;

        case BroadcastType::JobResult:
        {
            std::println("Hive broadcasting job result from Bee {}: {}",
                         sender_id, payload);
        }
        break;
    }
}

void
Hive::assign_jobs_to_bees()
{
    std::lock_guard<std::mutex> jobs_lock(m_jobs_mutex);
    std::lock_guard<std::mutex> bees_lock(m_bees_mutex);

    if (m_pending_jobs_queue.empty() || m_bees.empty())
        return;

    // We only want to loop through the queue ONCE per call
    static size_t bee_index = 0;

    for (size_t i = 0; i < m_pending_jobs_queue.size(); ++i)
    {
        if (m_pending_jobs_queue.empty())
            break;

        JobId job_id = m_pending_jobs_queue.front();
        m_pending_jobs_queue.pop();

        // Find the bee using your round-robin index
        auto &bee = m_bees[bee_index];
        bee_index = (bee_index + 1) % m_bees.size();

        if (bee->is_available())
        {
            Job *job = m_all_jobs[job_id].get();
            if (!job)
            {
                std::cerr << "Error: Job ID " << job_id
                          << " not found in all_jobs map\n";
                continue;
            }

            bee->assign_job(job);
            std::println("Assigned Job {} to Bee {}", job_id, bee->id());
        }
        else
        {
            // Put it back at the end of the line and STOP trying for now
            // OR keep it in the queue for the next assign_jobs_to_bees() call
            m_pending_jobs_queue.push(job_id);
        }
    }
}
