#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace ftxui
{
class ScreenInteractive;
}

class Monitor
{
public:
    Monitor(const std::string &host, const std::string &port);
    void run();

private:
    struct BeeInfo
    {
        uint64_t id;
        bool is_idle;
        bool is_benchmarking{false};
        std::optional<uint64_t> current_job;
        std::string current_job_name;
        std::optional<int64_t> job_start_ms;
        double cpu_gflops{0};
        double mem_bandwidth_gbps{0};
        std::optional<uint64_t> last_job_id;
        std::optional<int> last_exit_code;
        std::string last_output;
        std::string hostname;
        std::string os;
    };

    struct State
    {
        std::vector<BeeInfo> bees;
        int pending{0};
        int running{0};
        int completed{0};
        bool connected{false};
        std::string error;
    };

    void connect_and_read();

    std::string m_host;
    std::string m_port;
    std::mutex m_state_mutex;
    State m_state;
    std::atomic<ftxui::ScreenInteractive *> m_screen{nullptr};
};
