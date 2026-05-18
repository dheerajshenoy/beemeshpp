#pragma once

#include <nlohmann/json.hpp>
#include <string>

struct JobRequirements
{
    std::string name;
    unsigned    min_cpus{0};          // 0 = no requirement
    unsigned    min_mem_mb{0};        // 0 = no requirement
    bool        requires_gpu{false};
    std::string target_hostname;      // empty = any bee
    double      min_gflops{0.0};      // 0 = no requirement
    double      min_mem_bw_gbps{0.0}; // 0 = no requirement
};

inline void
to_json(nlohmann::json &j, const JobRequirements &r)
{
    j = nlohmann::json{{"name", r.name},
                       {"min_cpus", r.min_cpus},
                       {"min_mem_mb", r.min_mem_mb},
                       {"requires_gpu", r.requires_gpu},
                       {"target_hostname", r.target_hostname},
                       {"min_gflops", r.min_gflops},
                       {"min_mem_bw_gbps", r.min_mem_bw_gbps}};
}

inline void
from_json(const nlohmann::json &j, JobRequirements &r)
{
    r.name            = j.value("name", "");
    r.min_cpus        = j.value("min_cpus", 0u);
    r.min_mem_mb      = j.value("min_mem_mb", 0u);
    r.requires_gpu    = j.value("requires_gpu", false);
    r.target_hostname = j.value("target_hostname", "");
    r.min_gflops      = j.value("min_gflops", 0.0);
    r.min_mem_bw_gbps = j.value("min_mem_bw_gbps", 0.0);
}
