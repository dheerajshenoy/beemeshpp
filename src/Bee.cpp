#include "Bee.hpp"

#include "BenchmarkResult.hpp"
#include "MessageType.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <thread>
#include <vector>

static constexpr int OUTPUT_FLUSH_MS = 200; // max ms between output chunks

#if !defined(_WIN32)
    #include <sys/wait.h>
#endif

Bee::Bee(const std::string &auth_token, const std::string &host,
         const std::string &port)
    : m_auth_token(auth_token), m_host(host), m_port(port),
      m_socket(m_io_context)
{
    init_connection();
}

Bee::~Bee()
{
    std::println("Bee [id: {}] is shutting down...", m_id);
}

void
Bee::init_connection()
{
    try
    {
        tcp::resolver resolver(m_io_context);
        asio::connect(m_socket, resolver.resolve(m_host, m_port));
        std::println("Bee is connecting to Hive at {}:{} with auth token '{}'",
                     m_host, m_port, m_auth_token);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to connect to Hive: " << e.what() << "\n"
                  << "Make sure the Hive is running and the host/port/auth "
                     "token are correct.\n";
        exit(1);
    }
}

void
Bee::enqueue_write(std::string msg)
{
    m_write_queue.push_back(std::move(msg));
    if (!m_writing)
        do_write();
}

void
Bee::do_write()
{
    if (m_write_queue.empty())
    {
        m_writing = false;
        return;
    }
    m_writing         = true;
    const auto &front = m_write_queue.front();
    asio::async_write(m_socket, asio::buffer(front),
                      [this](const std::error_code &ec, std::size_t)
    {
        m_write_queue.pop_front();
        if (ec)
            std::cerr << "Write error: " << ec.message() << "\n";
        do_write();
    });
}

void
Bee::run()
{
    nlohmann::json reg
        = {{"type", Utils::to_string(MessageType::BEE_REGISTRATION)},
           {"data",
            {{"auth_token", m_auth_token},
             {"device_info", nlohmann::json(Utils::get_host_info())}}}};

    enqueue_write(reg.dump() + MSG_DELIMITER);
    read_from_hive();
    m_io_context.run();
}

void
Bee::start_job()
{
    if (!m_job)
        return;

    m_status = Status::Running;
}

void
Bee::stop_job()
{
    if (!m_job)
        return;

    m_status = Status::Stopped;
}

void
Bee::resume_job()
{
    if (!m_job)
        return;

    m_status = Status::Running;
}

static BenchmarkResult
run_benchmarks()
{
    BenchmarkResult r;

    // CPU: double-precision multiply-add loop
    {
        double a = 1.00001, b = 1.00002, c = 0.0;
        const long N = 200'000'000L;
        auto t0      = std::chrono::steady_clock::now();
        for (long i = 0; i < N; ++i)
            c = c * a + b;
        auto t1 = std::chrono::steady_clock::now();
        // prevent the loop being optimized away
        if (c == 0.0)
            std::println("");
        double secs  = std::chrono::duration<double>(t1 - t0).count();
        r.cpu_gflops = (2.0 * N) / (secs * 1e9);
    }

    // Memory bandwidth: large array copy (exceeds typical L3 cache)
    {
        const std::size_t N = 64ULL * 1024 * 1024; // 64M doubles = 512 MB
        std::vector<double> src(N, 1.0), dst(N, 0.0);
        auto t0 = std::chrono::steady_clock::now();
        std::copy(src.begin(), src.end(), dst.begin());
        auto t1 = std::chrono::steady_clock::now();
        if (dst[0] == 0.0)
            std::println(""); // prevent elision
        double secs = std::chrono::duration<double>(t1 - t0).count();
        // read + write = 2 * N * sizeof(double) bytes
        r.mem_bandwidth_gbps = (2.0 * N * sizeof(double)) / (secs * 1e9);
    }

    return r;
}

void
Bee::read_from_hive()
{

    asio::async_read_until(m_socket, asio::dynamic_buffer(m_buffer),
                           MSG_DELIMITER,
                           [this](const std::error_code &ec, std::size_t length)
    {
        if (ec)
        {
            std::cerr << "Error reading from Hive: " << ec.message() << "\n";
            return;
        }

        // Extract exactly one message worth of bytes
        std::string message(m_buffer.begin(), m_buffer.begin() + length);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + length);

        read_from_hive();
        handle_message(message);
    });
}

void
Bee::handle_message(const std::string &message)
{
    try
    {
        const auto json_msg = nlohmann::json::parse(message);
        const auto type     = Utils::message_type_from_string(
            json_msg.at("type").get<std::string>());

        switch (type)
        {
            case MessageType::BEE_ID_ASSIGNMENT:
                m_id = json_msg.at("data").at("bee_id").get<BeeId>();
                std::println("Bee assigned id: {}", m_id);
                break;

            case MessageType::JOB_ASSIGNMENT:
            {
                JobId job_id = json_msg.at("data").at("job_id").get<JobId>();
                std::string payload
                    = json_msg.at("data").at("payload").get<std::string>();

                std::println("Bee [id: {}] executing job {}: '{}'", m_id,
                             job_id, payload);
                m_status = Status::Running;

                // Run the job on a worker thread so the io_context stays
                // responsive (heartbeats, future assignments) during execution.
                std::string filename
                    = json_msg.at("data").value("filename", "");

                std::thread([this, job_id, payload, filename]()
                {
                    std::filesystem::path tmp_path;
                    if (!filename.empty())
                    {
                        tmp_path = std::filesystem::temp_directory_path()
                                   / ("beemesh_" + std::to_string(job_id) + "_"
                                      + filename);
                        std::ofstream f(tmp_path, std::ios::binary);
                        f << payload;
                        f.close();
#if !defined(_WIN32)
                        std::filesystem::permissions(
                            tmp_path,
                            std::filesystem::perms::owner_exec
                                | std::filesystem::perms::owner_read
                                | std::filesystem::perms::owner_write,
                            std::filesystem::perm_options::add);
#endif
                    }

                    std::string cmd
                        = tmp_path.empty() ? payload : tmp_path.string();

                    int exit_code = -1;

                    auto flush_chunk = [&](std::string chunk)
                    {
                        asio::post(m_io_context,
                                   [this, job_id, chunk = std::move(chunk)]()
                        {
                            nlohmann::json msg;
                            msg["type"]
                                = Utils::to_string(MessageType::JOB_OUTPUT);
                            msg["data"]
                                = {{"job_id", job_id}, {"chunk", chunk}};
                            enqueue_write(msg.dump() + MSG_DELIMITER);
                        });
                    };

#if defined(_WIN32)
                    FILE *pipe = _popen(cmd.c_str(), "r");
#else
                    FILE *pipe = popen(cmd.c_str(), "r");
#endif
                    if (pipe)
                    {
                        char buf[256];
                        std::string pending;
                        auto last_flush = std::chrono::steady_clock::now();

                        while (fgets(buf, sizeof(buf), pipe))
                        {
                            pending += buf;
                            auto now     = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<
                                               std::chrono::milliseconds>(
                                               now - last_flush)
                                               .count();
                            if (elapsed >= OUTPUT_FLUSH_MS
                                || pending.size() >= 4096)
                            {
                                flush_chunk(std::move(pending));
                                pending.clear();
                                last_flush = now;
                            }
                        }
                        if (!pending.empty())
                            flush_chunk(std::move(pending));

#if defined(_WIN32)
                        exit_code = _pclose(pipe);
#else
                        int status = pclose(pipe);
                        exit_code
                            = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
                    }

                    if (!tmp_path.empty())
                        std::filesystem::remove(tmp_path);

                    asio::post(m_io_context, [this, job_id, exit_code]()
                    {
                        m_status = Status::Idle;
                        nlohmann::json result;
                        result["type"]
                            = Utils::to_string(MessageType::JOB_RESULT);
                        result["data"]
                            = {{"job_id", job_id}, {"exit_code", exit_code}};
                        enqueue_write(result.dump() + MSG_DELIMITER);
                        std::println("Job {} finished (exit {})", job_id,
                                     exit_code);
                    });
                }).detach();

                break;
            }

            case MessageType::BENCHMARK_REQUEST:
            {
                std::println("Bee [id: {}] running benchmark…", m_id);
                std::thread([this]()
                {
                    auto result = run_benchmarks();
                    std::println("Bee [id: {}] benchmark done: {:.2f} GFLOPS, "
                                 "{:.1f} GB/s",
                                 m_id, result.cpu_gflops,
                                 result.mem_bandwidth_gbps);
                    asio::post(m_io_context, [this, result]()
                    {
                        nlohmann::json msg;
                        msg["type"]
                            = Utils::to_string(MessageType::BENCHMARK_RESULT);
                        msg["data"] = nlohmann::json(result);
                        enqueue_write(msg.dump() + MSG_DELIMITER);
                    });
                }).detach();
                break;
            }

            case MessageType::STATUS_UPDATE:
                break;

            case MessageType::STATUS_CHECK:
                std::println("Received status check from Hive");
                break;

            case MessageType::ERROR_REPORT:
                std::cerr << "Hive error: "
                          << json_msg.at("data").get<std::string>() << "\n";
                break;

            default:
                std::cerr << "Unexpected message type from Hive: "
                          << json_msg.at("type") << "\n";
                break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error handling message: " << e.what() << "\n";
    }
}
