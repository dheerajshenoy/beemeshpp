#pragma once

// Represents a single unit of work to be processed by the BeeMesh framework.
// Each Job can be executed by a Bee and may contain data, instructions, or
// tasks that need to be performed.

// Job = id + data
// Data - command or file to execute

#include <cstdint>
#include <string>

using JobId = uint64_t;

class Job
{
public:
    Job(JobId id, const std::string &data, const std::string &filename = {});
    ~Job();

    inline JobId id() const { return m_id; }

    inline const std::string &data() const { return m_data; }

    inline const std::string &filename() const { return m_filename; }

    // True when the payload is file content rather than a shell command.
    inline bool is_file() const { return !m_filename.empty(); }

private:
    JobId m_id;
    std::string m_data;
    std::string m_filename;
};
