#include "Launcher.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

#include <asio.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <print>
#include <sstream>

Launcher::Launcher(const std::string &host, const std::string &port,
                   const std::string &payload)
    : m_host(host), m_port(port), m_payload(payload)
{
}

void
Launcher::run()
{
    nlohmann::json data;

    std::filesystem::path path(m_payload);
    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open file: " + m_payload);

        std::ostringstream ss;
        ss << file.rdbuf();

        data["payload"]  = ss.str();
        data["filename"] = path.filename().string();

        std::println("Submitting file '{}' ({} bytes)", m_payload,
                     std::filesystem::file_size(path));
    }
    else
    {
        data["payload"] = m_payload;
        std::println("Submitting command: '{}'", m_payload);
    }

    asio::io_context io;
    asio::ip::tcp::socket socket(io);
    asio::ip::tcp::resolver resolver(io);
    asio::connect(socket, resolver.resolve(m_host, m_port));

    nlohmann::json msg;
    msg["type"] = Utils::to_string(MessageType::JOB_SUBMISSION);
    msg["data"] = data;
    asio::write(socket, asio::buffer(msg.dump() + "\n"));
}
