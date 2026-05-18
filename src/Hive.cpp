#include "Hive.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <print>

using tcp = asio::ip::tcp;

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
        tcp::resolver resolver(m_io_context);
        tcp::endpoint ep = *resolver.resolve(m_host, m_port).begin();

        m_acceptor.open(ep.protocol());
        m_acceptor.set_option(tcp::acceptor::reuse_address(true));
        m_acceptor.bind(ep);
        m_acceptor.listen();

        std::println("Hive listening on {}:{}", m_host, m_port);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to start Hive: " << e.what() << "\n";
        throw;
    }
}

void
Hive::run()
{
    do_accept();
    m_io_context.run();
}

void
Hive::do_accept()
{
    m_acceptor.async_accept([this](const std::error_code &ec, tcp::socket socket)
    {
        if (!ec)
            handle_connection(std::move(socket));
        do_accept();
    });
}

// Each incoming connection runs fully async from here — no blocking reads.
// A shared bool tracks whether the connection has completed registration so
// the same callback handles both the initial handshake and subsequent messages.
void
Hive::handle_connection(asio::ip::tcp::socket socket)
{
    auto conn          = std::make_shared<Connection>(std::move(socket));
    auto is_registered = std::make_shared<bool>(false);

    conn->read_async(
        [this, conn, is_registered](MessageType type, const nlohmann::json &data)
    {
        if (!*is_registered)
        {
            switch (type)
            {
                case MessageType::BEE_REGISTRATION:
                    *is_registered = register_bee(conn, data);
                    break;

                case MessageType::JOB_SUBMISSION:
                    add_job(data);
                    break;

                default:
                    std::println("Hive: unexpected first message type {}",
                                 static_cast<int>(type));
                    break;
            }
        }
        else
        {
            switch (type)
            {
                case MessageType::JOB_RESULT:
                    on_job_result(conn, data);
                    break;

                case MessageType::STATUS_UPDATE:
                    break;

                default:
                    break;
            }
        }
    },
        [this, conn]() { on_bee_disconnect(conn); });
}

// Returns true if authentication passed and the bee was registered.
bool
Hive::register_bee(const std::shared_ptr<Connection> &conn,
                   const nlohmann::json &data)
{
    std::string token = data.at("auth_token").get<std::string>();
    if (token != m_auth_token)
    {
        nlohmann::json err;
        err["type"] = Utils::to_string(MessageType::ERROR_REPORT);
        err["data"] = "invalid auth token";
        conn->write(err.dump() + "\n");
        std::println("Hive: rejected bee with bad auth token");
        return false;
    }

    static BeeId next_id = 1000;
    BeeId id             = next_id++;

    nlohmann::json response;
    response["type"] = Utils::to_string(MessageType::BEE_ID_ASSIGNMENT);
    response["data"] = {{"bee_id", id}};
    conn->write(response.dump() + "\n");

    {
        std::lock_guard<std::mutex> lock(m_bees_mutex);
        m_bees.push_back({conn, id, true, std::nullopt});
    }

    std::println("Bee {} registered", id);
    asio::post(m_io_context, [this]() { assign_jobs_to_bees(); });
    return true;
}

void
Hive::on_job_result(const std::shared_ptr<Connection> &conn,
                    const nlohmann::json &data)
{
    JobId job_id    = data.at("job_id").get<JobId>();
    std::string out = data.at("output").get<std::string>();
    std::println("Job {} result:\n{}", job_id, out);

    {
        std::lock_guard<std::mutex> lock(m_bees_mutex);
        for (auto &entry : m_bees)
        {
            if (entry.conn == conn)
            {
                entry.is_idle    = true;
                entry.current_job = std::nullopt;
                break;
            }
        }
    }

    asio::post(m_io_context, [this]() { assign_jobs_to_bees(); });
}

void
Hive::on_bee_disconnect(const std::shared_ptr<Connection> &conn)
{
    std::optional<JobId> requeue_job;

    {
        std::lock_guard<std::mutex> lock(m_bees_mutex);
        auto it = std::find_if(m_bees.begin(), m_bees.end(),
                               [&conn](const BeeEntry &e)
        { return e.conn == conn; });

        if (it == m_bees.end())
            return; // was a launcher or unregistered connection

        std::println("Bee {} disconnected", it->id);
        requeue_job = it->current_job;
        m_bees.erase(it);
    }

    if (requeue_job)
    {
        std::lock_guard<std::mutex> lock(m_jobs_mutex);
        m_pending_jobs.push(*requeue_job);
        std::println("Re-queued job {} from disconnected bee", *requeue_job);
        asio::post(m_io_context, [this]() { assign_jobs_to_bees(); });
    }
}

void
Hive::add_job(const nlohmann::json &data)
{
    static JobId next_id = 0;
    JobId id             = next_id++;

    std::string payload  = data.at("payload").get<std::string>();
    std::string filename = data.value("filename", "");

    if (filename.empty())
        std::println("Hive queued job {}: '{}'", id, payload);
    else
        std::println("Hive queued job {} (file: '{}')", id, filename);

    {
        std::lock_guard<std::mutex> lock(m_jobs_mutex);
        m_all_jobs[id] = std::make_unique<Job>(id, payload, filename);
        m_pending_jobs.push(id);
    }

    asio::post(m_io_context, [this]() { assign_jobs_to_bees(); });
}

void
Hive::assign_jobs_to_bees()
{
    std::lock_guard<std::mutex> jobs_lock(m_jobs_mutex);
    std::lock_guard<std::mutex> bees_lock(m_bees_mutex);

    for (auto &entry : m_bees)
    {
        if (m_pending_jobs.empty())
            break;
        if (!entry.is_idle)
            continue;

        JobId job_id = m_pending_jobs.front();
        m_pending_jobs.pop();

        auto &job = m_all_jobs.at(job_id);

        nlohmann::json msg;
        msg["type"] = Utils::to_string(MessageType::JOB_ASSIGNMENT);
        msg["data"] = {{"job_id", job_id},
                       {"payload", job->data()},
                       {"filename", job->filename()}};
        entry.conn->write(msg.dump() + "\n");

        entry.is_idle    = false;
        entry.current_job = job_id;

        std::println("Assigned job {} to bee {}", job_id, entry.id);
    }
}

void
Hive::handle_heartbeat()
{
    std::lock_guard<std::mutex> lock(m_bees_mutex);
    for (const auto &entry : m_bees)
    {
        nlohmann::json msg;
        msg["type"] = Utils::to_string(MessageType::STATUS_CHECK);
        msg["data"] = "ualive?";
        entry.conn->write(msg.dump() + "\n");
    }
}
