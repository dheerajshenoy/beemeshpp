#pragma once

#include "Bee.hpp"
#include "BenchmarkResult.hpp"
#include "Connection.hpp"
#include "Job.hpp"

#include <asio.hpp>
#include <chrono>
#include <deque>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

struct BeeEntry
{
    std::shared_ptr<Connection> conn;
    BeeId id{0};
    bool is_idle{true};
    bool is_benchmarking{false};
    std::optional<JobId> current_job;
    std::string current_job_name;
    std::optional<std::chrono::system_clock::time_point> job_start_time;
    std::string hostname;
    std::string os;
    unsigned cpu_cores{0};
    unsigned ram_mb{0};
    bool has_gpu{false};
    BenchmarkResult benchmark;
    std::optional<JobId> last_completed_job;
    std::string last_job_output;
    std::optional<int> last_exit_code;
};

class Hive
{
public:
    Hive(const std::string &auth_token, const std::string &host,
         const std::string &port, bool benchmark = false);
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
    void on_benchmark_result(const std::shared_ptr<Connection> &conn,
                             const nlohmann::json &data);
    void on_bee_disconnect(const std::shared_ptr<Connection> &conn);

    void register_monitor(const std::shared_ptr<Connection> &conn);
    void on_monitor_disconnect(const std::shared_ptr<Connection> &conn);
    void broadcast_status();

    void add_job(const nlohmann::json &data);
    void assign_jobs_to_bees();
    void handle_heartbeat();

private:
    asio::io_context m_io_context;
    asio::ip::tcp::acceptor m_acceptor;
    std::string m_auth_token;
    std::string m_host;
    std::string m_port;
    bool m_benchmark{false};

    std::vector<BeeEntry> m_bees;
    std::mutex m_bees_mutex;

    std::deque<JobId> m_pending_jobs;
    std::unordered_map<JobId, std::unique_ptr<Job>> m_all_jobs;
    std::mutex m_jobs_mutex;
    int m_completed_jobs{0};

    std::shared_ptr<Connection> m_monitor;
    std::mutex m_monitor_mutex;
};
