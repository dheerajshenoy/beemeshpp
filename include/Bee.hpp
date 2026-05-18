#pragma once

#include "HostInfo.hpp"
#include "Job.hpp"

#include <asio.hpp>
#include <cstdint>
#include <deque>
#include <string>

using BeeId = uint64_t;
using asio::ip::tcp;

#define MSG_DELIMITER "\n"

class Bee
{
public:
    Bee(const std::string &auth_token, const std::string &host,
        const std::string &port);
    ~Bee();

    enum class Status
    {
        Idle,
        Running,
        Stopped,
        Completed,
        Failed
    };

    inline Status status() const
    {
        return m_status;
    }

    inline BeeId id() const
    {
        return m_id;
    }

    inline bool is_available() const
    {
        return m_status == Status::Idle;
    }

    void run();

    inline void assign_job(Job *job)
    {
        m_job = job;
    }

    void start_job();
    void stop_job();
    void resume_job();

private:
    void init_connection();
    void read_from_hive();
    void handle_message(const std::string &message);
    void enqueue_write(std::string msg);
    void do_write();

private:
    asio::io_context m_io_context;
    std::string m_auth_token;
    std::string m_host;
    std::string m_port;
    asio::ip::tcp::socket m_socket;
    BeeId m_id;
    HostInfo m_info;
    Job *m_job{nullptr};
    std::string m_buffer;
    Status m_status{Status::Idle};
    std::deque<std::string> m_write_queue;
    bool m_writing{false};
};
