#include "Job.hpp"

Job::Job(JobId id, const std::string &data, const std::string &filename,
         JobRequirements reqs)
    : m_id(id), m_data(data), m_filename(filename), m_requirements(std::move(reqs))
{
}

Job::~Job() {}
