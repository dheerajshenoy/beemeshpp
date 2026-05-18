#include "Launcher.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <print>

Launcher::Launcher(const std::string &host, const std::string &port,
                   const std::string &payload)
    : m_host(host), m_port(port), m_payload(payload)
{
}

void
Launcher::run()
{
    asio::io_context io;
    asio::ip::tcp::socket socket(io);
    asio::ip::tcp::resolver resolver(io);
    asio::connect(socket, resolver.resolve(m_host, m_port));

    nlohmann::json msg
        = {{"type", Utils::to_string(MessageType::JOB_SUBMISSION)},
           {"data", m_payload}};
    std::string out = msg.dump() + "\n";
    asio::write(socket, asio::buffer(out));

    std::println("Job submitted: '{}'", m_payload);
}
