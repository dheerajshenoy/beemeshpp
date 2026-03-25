#include "Job.hpp"

Job::Job(JobId id, const std::string &data) : m_id(id), m_data(data) {}

Job::~Job() {}
