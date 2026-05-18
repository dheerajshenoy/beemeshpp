#include "Monitor.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

#include <asio.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <nlohmann/json.hpp>
#include <thread>

#define REFRESH_INTERVAL_SECONDS 5

using namespace ftxui;

Monitor::Monitor(const std::string &host, const std::string &port)
    : m_host(host), m_port(port)
{
}

// Runs in a background thread: connects to the hive, sends MONITOR_CONNECT,
// then loops reading STATUS_SNAPSHOT messages and updating shared state.
void
Monitor::connect_and_read()
{
    try
    {
        asio::io_context io;
        asio::ip::tcp::socket socket(io);
        asio::ip::tcp::resolver resolver(io);

        // Throws immediately if the hive is not reachable.
        asio::connect(socket, resolver.resolve(m_host, m_port));

        // TCP connection established — show the TUI now, before any snapshot.
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

        std::string buf;
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
                    info.id       = bee.at("id").get<uint64_t>();
                    info.is_idle  = bee.at("is_idle").get<bool>();
                    info.hostname = bee.value("hostname", "");
                    info.os       = bee.value("os", "");
                    if (bee.contains("job_id"))
                        info.current_job = bee.at("job_id").get<uint64_t>();
                    if (bee.contains("job_start_ms"))
                        info.job_start_ms
                            = bee.at("job_start_ms").get<int64_t>();
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

void
Monitor::run()
{
    auto screen = ScreenInteractive::Fullscreen();
    m_screen.store(&screen);

    std::thread([this]() { connect_and_read(); }).detach();

    // Tick every second so elapsed-time columns stay live.
    std::thread([this, &screen]()
    {
        while (m_screen.load())
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(REFRESH_INTERVAL_SECONDS));
            if (auto *s = m_screen.load())
                s->PostEvent(Event::Custom);
        }
    }).detach();

    auto renderer = Renderer([this]() -> Element
    {
        State snap;
        {
            std::lock_guard lock(m_state_mutex);
            snap = m_state;
        }

        // ── title ──────────────────────────────────────────────────
        auto title
            = text(" BeeMesh Monitor ") | bold | color(Color::Cyan) | hcenter;

        // ── connection status ──────────────────────────────────────
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

        // ── bees table ─────────────────────────────────────────────
        Elements rows;
        rows.push_back(
            hbox({text(" ID  ") | bold | size(WIDTH, EQUAL, 6), separator(),
                  text(" Hostname          ") | bold | size(WIDTH, EQUAL, 20),
                  separator(),
                  text(" OS       ") | bold | size(WIDTH, EQUAL, 10),
                  separator(),
                  text(" Status  ") | bold | size(WIDTH, EQUAL, 10),
                  separator(),
                  text(" Job      ") | bold | size(WIDTH, EQUAL, 14),
                  separator(), text(" Elapsed ") | bold})
            | color(Color::White));
        rows.push_back(separator());

        if (snap.bees.empty())
        {
            rows.push_back(text(" No bees connected ") | color(Color::GrayDark)
                           | hcenter);
        }
        else
        {
            auto now_ms
                = std::chrono::duration_cast<std::chrono::milliseconds>(
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

            for (const auto &bee : snap.bees)
            {
                auto status_color = bee.is_idle ? Color::Green : Color::Yellow;
                auto job_str = bee.current_job
                                   ? " #" + std::to_string(*bee.current_job)
                                   : " -";
                auto elapsed_str
                    = bee.job_start_ms
                          ? " " + fmt_elapsed(now_ms - *bee.job_start_ms)
                          : " -";
                auto host = bee.hostname.empty() ? "unknown" : bee.hostname;
                auto os   = bee.os.empty() ? "-" : bee.os;
                rows.push_back(hbox(
                    {text(" " + std::to_string(bee.id) + " ")
                         | size(WIDTH, EQUAL, 6),
                     separator(),
                     text(" " + host + " ") | size(WIDTH, EQUAL, 20),
                     separator(), text(" " + os + " ") | size(WIDTH, EQUAL, 10),
                     separator(),
                     text(" " + std::string(bee.is_idle ? "idle" : "busy")
                          + " ")
                         | color(status_color) | size(WIDTH, EQUAL, 10),
                     separator(), text(job_str) | size(WIDTH, EQUAL, 14),
                     separator(), text(elapsed_str) | color(Color::Yellow)}));
            }
        }

        auto bees_box = vbox(rows) | border;

        // ── stats bar ──────────────────────────────────────────────
        auto stats = hbox({
            text(" Pending ") | bold,
            text(std::to_string(snap.pending)) | color(Color::Blue),
            text("   Running ") | bold,
            text(std::to_string(snap.running)) | color(Color::Yellow),
            text("   Completed ") | bold,
            text(std::to_string(snap.completed)) | color(Color::Green),
            text("  "),
        });

        // ── hint ───────────────────────────────────────────────────
        auto hint = text(" q  quit ") | color(Color::GrayDark) | align_right;

        return vbox({title, separator(), bees_box, separator(), stats,
                     separator(), hint})
               | border;
    });

    // Consume the custom redraw events posted from the network thread;
    // pass everything else through for default handling (resize, etc.).
    auto component = CatchEvent(renderer, [&screen](Event event)
    {
        if (event == Event::Character('q') || event == Event::Character('Q'))
        {
            screen.ExitLoopClosure()();
            return true;
        }
        return event == Event::Custom;
    });

    screen.Loop(component);
    m_screen.store(nullptr);
}
