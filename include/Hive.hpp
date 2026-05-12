#pragma once

#include "Bee.hpp"
#include "Connection.hpp"
#include "Job.hpp"

#include <asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>

enum class BroadcastType
{
    StatusUpdate = 0,
    JobResult
};

using BeeList = std::vector<std::shared_ptr<Bee>>;

class Hive
{
public:
    Hive(const std::string &auth_token, const std::string &host,
         const std::string &port);
    ~Hive();

    Hive(Hive *);

    void run();

    // inline const BeeList &bees() const
    // {
    //     return m_bees;
    // }

    void add_job(std::unique_ptr<Job> job);

private:
    void init_connection();
    void add_job(const std::string &data);
    void add_bee(const std::shared_ptr<Connection> &conn,
                 const nlohmann::json &bee_info);
    void send_id_to_bee(const std::shared_ptr<Connection> &conn);
    void handle_connection(asio::ip::tcp::socket socket);
    void do_accept();
    void assign_jobs_to_bees();
    void handle_heartbeat();

private:
    asio::io_context m_io_context;
    asio::ip::tcp::acceptor m_acceptor;
    std::string m_auth_token;
    std::string m_host;
    std::string m_port;
    // BeeList m_bees;
    std::vector<std::shared_ptr<Connection>> m_bee_connections;
    std::mutex m_bees_mutex;
    std::mutex m_jobs_mutex;
    std::queue<JobId> m_pending_jobs_queue;
    std::unordered_map<JobId, std::unique_ptr<Job>> m_all_jobs;
};
