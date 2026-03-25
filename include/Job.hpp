#pragma once

// Represents a single unit of work to be processed by the BeeMesh framework.
// Each Job can be executed by a Bee and may contain data, instructions, or
// tasks that need to be performed.

// Job = id + data (basically)
// Data - command or file to execute

#include <cstdint>
#include <string>

using JobId = uint64_t;

class Job
{
public:
    Job(JobId id, const std::string &data);
    ~Job();

    enum class Status
    {
        None = 0,
        Pending,
        InProgress,
        Completed,
        Failed
    };

    inline JobId id() const
    {
        return m_id;
    }

    inline void set_data(const std::string &data)
    {
        m_data = data;
    }

    inline const std::string &data() const
    {
        return m_data;
    }

    inline void set_status(Status status)
    {
        m_status = status;
    }

    inline Status status() const
    {
        return m_status;
    }

private:
    JobId m_id;
    std::string m_data;
    Status m_status{Status::None};
};
