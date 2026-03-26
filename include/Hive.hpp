#pragma once

#include "Bee.hpp"
#include "Endpoint.hpp"
#include "Job.hpp"

#include <asio.hpp>
#include <memory>
#include <queue>

using BeeList = std::vector<std::unique_ptr<Bee>>;

class Hive
{
public:
    Hive(asio::io_context &io_context, const std::string &auth_token,
         const Endpoint &endpoint);
    ~Hive();

    Hive(Hive *);

    void run();

    inline const BeeList &bees() const
    {
        return m_bees;
    }

    void add_job(std::unique_ptr<Job> job);
    void broadcast_payload(MessageType type, const std::string &data,
                           BeeId sender_id);

private:
    void add_bee(asio::ip::tcp::socket socket);
    void handle_connection(asio::ip::tcp::socket socket);
    void do_accept();

private:
    asio::io_context &m_io_context;
    asio::ip::tcp::acceptor m_acceptor;
    std::string m_auth_token;
    Endpoint m_endpoint;
    BeeList m_bees;
    std::mutex m_bees_mutex;
    std::mutex m_jobs_mutex;
    std::queue<JobId> m_pending_jobs_queue;
    std::vector<std::unique_ptr<Job>> m_all_jobs;
};
