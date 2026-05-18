#pragma once

#include <atomic>
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
        std::optional<uint64_t> current_job;
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
