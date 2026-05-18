#include "Bee.hpp"

#include "MessageType.hpp"
#include "Utils.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <thread>

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
    std::println("Bee is connecting to Hive at {}:{} with auth token '{}'",
                 m_host, m_port, m_auth_token);
    tcp::resolver resolver(m_io_context);
    asio::connect(m_socket, resolver.resolve(m_host, m_port));
}

// Inside Bee.cpp
void
Bee::run()
{
    nlohmann::json data
        = {{"type", Utils::to_string(MessageType::BEE_REGISTRATION)},
           {"data",
            {{"auth_token", m_auth_token},
             {"device_info", nlohmann::json(Utils::get_host_info())}}}};

    asio::async_write(m_socket, asio::buffer(data.dump() + MSG_DELIMITER),
                      [this](const std::error_code &ec, std::size_t)
    {
        if (ec)
        {
            std::cerr << "Error sending registration to Hive: " << ec.message()
                      << "\n";
            return;
        }
        read_from_hive();
    });

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
                JobId job_id        = json_msg.at("data").at("job_id").get<JobId>();
                std::string payload = json_msg.at("data").at("payload").get<std::string>();

                std::println("Bee [id: {}] executing job {}: '{}'", m_id, job_id, payload);
                m_status = Status::Running;

                // Run the job on a worker thread so the io_context stays
                // responsive (heartbeats, future assignments) during execution.
                std::string filename =
                    json_msg.at("data").value("filename", "");

                std::thread([this, job_id, payload, filename]()
                {
                    std::filesystem::path tmp_path;
                    if (!filename.empty())
                    {
                        tmp_path = std::filesystem::temp_directory_path()
                                   / ("beemesh_" + std::to_string(job_id)
                                      + "_" + filename);
                        std::ofstream f(tmp_path, std::ios::binary);
                        f << payload;
                        f.close();
                        std::filesystem::permissions(
                            tmp_path,
                            std::filesystem::perms::owner_exec
                                | std::filesystem::perms::owner_read
                                | std::filesystem::perms::owner_write,
                            std::filesystem::perm_options::add);
                    }

                    std::string cmd =
                        tmp_path.empty() ? payload : tmp_path.string();

                    std::string output;
                    FILE *pipe = popen(cmd.c_str(), "r");
                    if (pipe)
                    {
                        char buf[256];
                        while (fgets(buf, sizeof(buf), pipe))
                            output += buf;
                        pclose(pipe);
                    }

                    if (!tmp_path.empty())
                        std::filesystem::remove(tmp_path);

                    // Post result back to the io_context thread for safe socket access.
                    asio::post(m_io_context,
                               [this, job_id, output = std::move(output)]()
                    {
                        m_status = Status::Idle;

                        nlohmann::json result;
                        result["type"] = Utils::to_string(MessageType::JOB_RESULT);
                        result["data"] = {{"job_id", job_id}, {"output", output}};

                        asio::async_write(
                            m_socket,
                            asio::buffer(result.dump() + MSG_DELIMITER),
                            [job_id](const std::error_code &ec, std::size_t)
                        {
                            if (!ec)
                                std::println("Sent result for job {}", job_id);
                            else
                                std::cerr << "Error sending result: "
                                          << ec.message() << "\n";
                        });
                    });
                }).detach();

                break;
            }

            case MessageType::STATUS_UPDATE:
                // handle status
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
