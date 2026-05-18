#include "Job.hpp"

Job::Job(JobId id, const std::string &data, const std::string &filename)
    : m_id(id), m_data(data), m_filename(filename)
{
}

Job::~Job() {}
