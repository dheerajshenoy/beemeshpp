#pragma once

#include <nlohmann/json.hpp>

struct BenchmarkResult
{
    double cpu_gflops{0};          // double-precision GFLOPS
    double mem_bandwidth_gbps{0};  // memory copy bandwidth GB/s
};

inline void
to_json(nlohmann::json &j, const BenchmarkResult &b)
{
    j = nlohmann::json{{"cpu_gflops", b.cpu_gflops},
                       {"mem_bandwidth_gbps", b.mem_bandwidth_gbps}};
}

inline void
from_json(const nlohmann::json &j, BenchmarkResult &b)
{
    b.cpu_gflops         = j.value("cpu_gflops", 0.0);
    b.mem_bandwidth_gbps = j.value("mem_bandwidth_gbps", 0.0);
}
