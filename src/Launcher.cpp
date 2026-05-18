#include "Launcher.hpp"

#include "JobRequirements.hpp"
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

// Parse memory strings like "16G", "512M", "1024" (bare = MB).
static unsigned
parse_mem_mb(const std::string &s)
{
    if (s.empty()) return 0;
    char suffix = static_cast<char>(std::toupper(static_cast<unsigned char>(s.back())));
    unsigned val = static_cast<unsigned>(std::stoul(
        (suffix == 'G' || suffix == 'M') ? s.substr(0, s.size() - 1) : s));
    return (suffix == 'G') ? val * 1024 : val;
}

static JobRequirements
parse_beemesh_directives(const std::string &content)
{
    JobRequirements reqs;
    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line))
    {
        // Stop scanning once we're past the header comments.
        if (!line.empty() && line[0] != '#' && line.substr(0, 2) != "//")
            break;

        const std::string prefix = "#BEEMESH";
        if (line.rfind(prefix, 0) != 0)
            continue;

        std::istringstream ls(line.substr(prefix.size()));
        std::string key, val;
        ls >> key;

        if (key == "--name")
        {
            std::getline(ls >> std::ws, val);
            reqs.name = val;
        }
        else if (key == "--cpus")
        {
            ls >> val;
            reqs.min_cpus = static_cast<unsigned>(std::stoul(val));
        }
        else if (key == "--mem")
        {
            ls >> val;
            reqs.min_mem_mb = parse_mem_mb(val);
        }
        else if (key == "--gpu")
        {
            reqs.requires_gpu = true;
        }
        else if (key == "--target")
        {
            ls >> reqs.target_hostname;
        }
        else if (key == "--min-gflops")
        {
            ls >> val;
            reqs.min_gflops = std::stod(val);
        }
        else if (key == "--min-mem-bw")
        {
            ls >> val;
            reqs.min_mem_bw_gbps = std::stod(val);
        }
    }

    return reqs;
}

void
Launcher::run()
{
    nlohmann::json data;
    JobRequirements reqs;

    std::filesystem::path path(m_payload);
    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open file: " + m_payload);

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        reqs             = parse_beemesh_directives(content);
        data["payload"]  = content;
        data["filename"] = path.filename().string();

        std::println("Submitting file '{}' ({} bytes)", m_payload,
                     std::filesystem::file_size(path));
    }
    else
    {
        data["payload"] = m_payload;
        std::println("Submitting command: '{}'", m_payload);
    }

    if (!reqs.name.empty() || reqs.min_cpus > 0 || reqs.min_mem_mb > 0
        || reqs.requires_gpu || !reqs.target_hostname.empty())
    {
        data["requirements"] = nlohmann::json(reqs);

        if (!reqs.name.empty())
            std::println("  name:        {}", reqs.name);
        if (reqs.min_cpus > 0)
            std::println("  cpus:        >= {}", reqs.min_cpus);
        if (reqs.min_mem_mb > 0)
            std::println("  mem:         >= {} MB", reqs.min_mem_mb);
        if (reqs.requires_gpu)
            std::println("  gpu:         required");
        if (!reqs.target_hostname.empty())
            std::println("  target:      {}", reqs.target_hostname);
        if (reqs.min_gflops > 0.0)
            std::println("  min-gflops:  >= {:.2f}", reqs.min_gflops);
        if (reqs.min_mem_bw_gbps > 0.0)
            std::println("  min-mem-bw:  >= {:.2f} GB/s", reqs.min_mem_bw_gbps);
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
