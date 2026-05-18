#pragma once

#include "Bee.hpp"
#include "Connection.hpp"
#include "Job.hpp"

#include <asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <queue>
#include <unordered_map>

struct BeeEntry
{
    std::shared_ptr<Connection> conn;
    BeeId id{0};
    bool is_idle{true};
    std::optional<JobId> current_job;
};

class Hive
{
public:
    Hive(const std::string &auth_token, const std::string &host,
         const std::string &port);
    ~Hive();

    void run();

private:
    void init_connection();
    void do_accept();
    void handle_connection(asio::ip::tcp::socket socket);

    bool register_bee(const std::shared_ptr<Connection> &conn,
                      const nlohmann::json &data);
    void on_job_result(const std::shared_ptr<Connection> &conn,
                       const nlohmann::json &data);
    void on_bee_disconnect(const std::shared_ptr<Connection> &conn);

    void add_job(const nlohmann::json &data);
    void assign_jobs_to_bees();
    void handle_heartbeat();

private:
    asio::io_context m_io_context;
    asio::ip::tcp::acceptor m_acceptor;
    std::string m_auth_token;
    std::string m_host;
    std::string m_port;

    std::vector<BeeEntry> m_bees;
    std::mutex m_bees_mutex;

    std::queue<JobId> m_pending_jobs;
    std::unordered_map<JobId, std::unique_ptr<Job>> m_all_jobs;
    std::mutex m_jobs_mutex;
};
