#include "Monitor.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <asio.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#define REFRESH_INTERVAL_SECONDS 1

using namespace ftxui;

Monitor::Monitor(const std::string &host, const std::string &port)
    : m_host(host), m_port(port)
{
}

void
Monitor::connect_and_read()
{
    try
    {
        asio::io_context io;
        asio::ip::tcp::socket socket(io);
        asio::ip::tcp::resolver resolver(io);

        asio::connect(socket, resolver.resolve(m_host, m_port));

        {
            std::lock_guard lock(m_state_mutex);
            m_state.connected = true;
        }
        if (auto *s = m_screen.load())
            s->PostEvent(Event::Custom);

        nlohmann::json hello;
        hello["type"] = Utils::to_string(MessageType::MONITOR_CONNECT);
        hello["data"] = nullptr;
        asio::write(socket, asio::buffer(hello.dump() + "\n"));

        while (true)
        {
            asio::streambuf sbuf;
            asio::read_until(socket, sbuf, "\n");
            std::istream is(&sbuf);
            std::string line;
            std::getline(is, line);

            auto json_msg = nlohmann::json::parse(line);
            auto type     = Utils::message_type_from_string(
                json_msg.at("type").get<std::string>());

            if (type == MessageType::STATUS_SNAPSHOT)
            {
                const auto &data = json_msg.at("data");

                std::lock_guard lock(m_state_mutex);
                m_state.bees.clear();
                for (const auto &bee : data.at("bees"))
                {
                    BeeInfo info;
                    info.id          = bee.at("id").get<uint64_t>();
                    info.is_idle     = bee.at("is_idle").get<bool>();
                    info.hostname    = bee.value("hostname", "");
                    info.os          = bee.value("os", "");
                    info.last_output = bee.value("last_output", "");
                    if (bee.contains("job_id"))
                        info.current_job = bee.at("job_id").get<uint64_t>();
                    info.current_job_name = bee.value("job_name", "");
                    if (bee.contains("job_start_ms"))
                        info.job_start_ms = bee.at("job_start_ms").get<int64_t>();
                    if (bee.contains("last_job_id"))
                        info.last_job_id = bee.at("last_job_id").get<uint64_t>();
                    if (bee.contains("last_exit_code"))
                        info.last_exit_code = bee.at("last_exit_code").get<int>();
                    m_state.bees.push_back(info);
                }
                m_state.pending   = data.at("pending").get<int>();
                m_state.running   = data.at("running").get<int>();
                m_state.completed = data.at("completed").get<int>();
            }

            if (auto *s = m_screen.load())
                s->PostEvent(Event::Custom);
        }
    }
    catch (const std::exception &e)
    {
        std::lock_guard lock(m_state_mutex);
        m_state.connected = false;
        m_state.error     = e.what();
        if (auto *s = m_screen.load())
            s->PostEvent(Event::Custom);
    }
}

static std::vector<std::string>
split_lines(const std::string &s)
{
    std::vector<std::string> lines;
    std::istringstream ss(s);
    std::string line;
    while (std::getline(ss, line))
        lines.push_back(line);
    return lines;
}

void
Monitor::run()
{
    auto screen = ScreenInteractive::Fullscreen();
    m_screen.store(&screen);

    int  selected      = 0;
    bool show_detail   = false;
    int  output_scroll = 0;

    std::thread([this]() { connect_and_read(); }).detach();

    std::thread([this]()
    {
        while (m_screen.load())
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(REFRESH_INTERVAL_SECONDS));
            if (auto *s = m_screen.load())
                s->PostEvent(Event::Custom);
        }
    }).detach();

    auto renderer = Renderer([&]() -> Element
    {
        State snap;
        {
            std::lock_guard lock(m_state_mutex);
            snap = m_state;
        }

        int n = static_cast<int>(snap.bees.size());
        if (n > 0)
            selected = std::clamp(selected, 0, n - 1);

        auto title = text(" BeeMesh Monitor ") | bold | color(Color::Cyan)
                     | hcenter;

        if (!snap.error.empty())
        {
            return vbox({title, separator(),
                         text(" Could not connect to hive: " + snap.error)
                             | color(Color::Red) | hcenter,
                         text(" (Is the hive running?) ")
                             | color(Color::GrayDark) | hcenter})
                   | border;
        }

        if (!snap.connected)
        {
            return vbox({title, separator(),
                         text(" Connecting to " + m_host + ":" + m_port + "…")
                             | color(Color::Yellow) | hcenter})
                   | border;
        }

        // ── detail panel ───────────────────────────────────────────
        if (show_detail && n > 0)
        {
            const auto &bee = snap.bees[selected];

            auto host = bee.hostname.empty() ? "unknown" : bee.hostname;
            auto info = hbox({
                text(" Bee #" + std::to_string(bee.id)) | bold,
                text("  " + host) | color(Color::Cyan),
                text("  " + bee.os) | color(Color::GrayDark),
            });

            std::string job_label;
            Color job_color = Color::Yellow;
            if (bee.current_job)
            {
                job_label = "Running job #" + std::to_string(*bee.current_job);
                if (!bee.current_job_name.empty())
                    job_label += "  \"" + bee.current_job_name + "\"";
            }
            else if (bee.last_job_id)
            {
                job_label = "Last job #" + std::to_string(*bee.last_job_id);
                if (bee.last_exit_code)
                {
                    job_label += "  exit " + std::to_string(*bee.last_exit_code);
                    job_color = (*bee.last_exit_code == 0) ? Color::Green : Color::Red;
                }
            }
            else
            {
                job_label = "No jobs run yet";
                job_color = Color::GrayDark;
            }

            auto job_line = text(" " + job_label + " ") | color(job_color);

            auto lines         = split_lines(bee.last_output);
            constexpr int kMax = 20;
            int total          = static_cast<int>(lines.size());
            int scroll         = std::clamp(output_scroll, 0,
                                    std::max(0, total - kMax));
            output_scroll      = scroll;

            Elements output_rows;
            if (lines.empty())
            {
                output_rows.push_back(
                    text(" (no output) ") | color(Color::GrayDark));
            }
            else
            {
                int end = std::min(scroll + kMax, total);
                for (int i = scroll; i < end; ++i)
                    output_rows.push_back(text(" " + lines[i]));
            }

            std::string scroll_hint;
            if (total > kMax)
                scroll_hint = " lines " + std::to_string(scroll + 1) + "-"
                              + std::to_string(std::min(scroll + kMax, total))
                              + " of " + std::to_string(total) + " ";

            auto output_box = vbox(output_rows) | border;

            auto hint = hbox({
                text(" esc/enter  back") | color(Color::GrayDark),
                scroll_hint.empty() ? text("") : text(scroll_hint) | color(Color::GrayDark) | align_right,
            });

            return vbox({title, separator(), info, separator(), job_line,
                         separator(), output_box, separator(), hint})
                   | border;
        }

        // ── bees table ─────────────────────────────────────────────
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

        auto fmt_elapsed = [](int64_t ms) -> std::string
        {
            int s = static_cast<int>(ms / 1000);
            if (s < 60)
                return std::to_string(s) + "s";
            int m = s / 60;
            s %= 60;
            if (m < 60)
                return std::to_string(m) + "m " + std::to_string(s) + "s";
            int h = m / 60;
            m %= 60;
            return std::to_string(h) + "h " + std::to_string(m) + "m";
        };

        Elements rows;
        rows.push_back(
            hbox({text(" ID  ") | bold | size(WIDTH, EQUAL, 6),
                  separator(),
                  text(" Hostname          ") | bold | size(WIDTH, EQUAL, 20),
                  separator(),
                  text(" OS       ") | bold | size(WIDTH, EQUAL, 10),
                  separator(),
                  text(" Status  ") | bold | size(WIDTH, EQUAL, 10),
                  separator(),
                  text(" Job      ") | bold | size(WIDTH, EQUAL, 14),
                  separator(),
                  text(" Elapsed ") | bold | size(WIDTH, EQUAL, 10),
                  separator(),
                  text(" Exit ") | bold})
            | color(Color::White));
        rows.push_back(separator());

        if (snap.bees.empty())
        {
            rows.push_back(text(" No bees connected ") | color(Color::GrayDark)
                           | hcenter);
        }
        else
        {
            for (int i = 0; i < n; ++i)
            {
                const auto &bee       = snap.bees[i];
                auto status_color     = bee.is_idle ? Color::Green : Color::Yellow;
                std::string job_str = " -";
                if (bee.current_job)
                {
                    job_str = " #" + std::to_string(*bee.current_job);
                    if (!bee.current_job_name.empty())
                        job_str += " " + bee.current_job_name;
                }
                auto elapsed_str      = bee.job_start_ms
                                            ? " " + fmt_elapsed(now_ms - *bee.job_start_ms)
                                            : " -";
                auto host             = bee.hostname.empty() ? "unknown" : bee.hostname;
                auto os               = bee.os.empty() ? "-" : bee.os;

                std::string exit_str = " -";
                Color exit_color     = Color::GrayDark;
                if (bee.last_exit_code)
                {
                    exit_str  = " " + std::to_string(*bee.last_exit_code);
                    exit_color = (*bee.last_exit_code == 0) ? Color::Green : Color::Red;
                }

                auto row = hbox(
                    {text(" " + std::to_string(bee.id) + " ")
                         | size(WIDTH, EQUAL, 6),
                     separator(),
                     text(" " + host + " ") | size(WIDTH, EQUAL, 20),
                     separator(),
                     text(" " + os + " ") | size(WIDTH, EQUAL, 10),
                     separator(),
                     text(" " + std::string(bee.is_idle ? "idle" : "busy") + " ")
                         | color(status_color) | size(WIDTH, EQUAL, 10),
                     separator(),
                     text(job_str) | size(WIDTH, EQUAL, 14),
                     separator(),
                     text(elapsed_str) | color(Color::Yellow) | size(WIDTH, EQUAL, 10),
                     separator(),
                     text(exit_str) | color(exit_color)});

                if (i == selected)
                    row = row | bgcolor(Color::GrayDark);

                rows.push_back(row);
            }
        }

        auto bees_box = vbox(rows) | border;

        auto stats = hbox({
            text(" Pending ") | bold,
            text(std::to_string(snap.pending)) | color(Color::Blue),
            text("   Running ") | bold,
            text(std::to_string(snap.running)) | color(Color::Yellow),
            text("   Completed ") | bold,
            text(std::to_string(snap.completed)) | color(Color::Green),
            text("  "),
        });

        auto hint = hbox({
            text(" j/k ↑/↓  navigate") | color(Color::GrayDark),
            text("   enter  details") | color(Color::GrayDark),
            text("   q  quit") | color(Color::GrayDark),
        }) | align_right;

        return vbox({title, separator(), bees_box, separator(), stats,
                     separator(), hint})
               | border;
    });

    auto component = CatchEvent(renderer, [&](Event event)
    {
        if (event == Event::Character('q') || event == Event::Character('Q'))
        {
            screen.ExitLoopClosure()();
            return true;
        }

        State snap;
        {
            std::lock_guard lock(m_state_mutex);
            snap = m_state;
        }
        int n = static_cast<int>(snap.bees.size());

        if (show_detail)
        {
            if (event == Event::Escape || event == Event::Return)
            {
                show_detail   = false;
                output_scroll = 0;
                return true;
            }
            if (event == Event::ArrowUp || event == Event::Character('k'))
            {
                if (output_scroll > 0) --output_scroll;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j'))
            {
                ++output_scroll;
                return true;
            }
            return event == Event::Custom;
        }

        if (event == Event::ArrowUp || event == Event::Character('k'))
        {
            if (n > 0) selected = (selected - 1 + n) % n;
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('j'))
        {
            if (n > 0) selected = (selected + 1) % n;
            return true;
        }
        if (event == Event::Return)
        {
            if (n > 0) { show_detail = true; output_scroll = 0; }
            return true;
        }

        return event == Event::Custom;
    });

    screen.Loop(component);
    m_screen.store(nullptr);
}
