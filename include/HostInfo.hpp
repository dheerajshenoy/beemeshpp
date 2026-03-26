#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

struct HostInfo
{
    std::string name;
    std::string OS;

    struct CPU
    {
        std::string name;
        unsigned n_cores   = 0;
        unsigned n_threads = 0;
        std::string arch;
    };

    struct GPU
    {
        std::string name;
        std::string vendor;
        unsigned VRAM_mb = 0;
    };

    std::vector<CPU> CPUs;
    std::vector<GPU> GPUs;
    unsigned RAM_mb;
};

// --- Serialization ---
inline void
to_json(json &j, const HostInfo::CPU &c)
{
    j = json{{"name", c.name},
             {"n_cores", c.n_cores},
             {"n_threads", c.n_threads},
             {"arch", c.arch}};
}

inline void
from_json(const json &j, HostInfo::CPU &c)
{
    j.at("name").get_to(c.name);
    j.at("n_cores").get_to(c.n_cores);
    j.at("n_threads").get_to(c.n_threads);
    j.at("arch").get_to(c.arch);
}

inline void
to_json(json &j, const HostInfo::GPU &g)
{
    j = json{{"name", g.name}, {"vendor", g.vendor}, {"VRAM_mb", g.VRAM_mb}};
}

inline void
from_json(const json &j, HostInfo::GPU &g)
{
    j.at("name").get_to(g.name);
    j.at("vendor").get_to(g.vendor);
    j.at("VRAM_mb").get_to(g.VRAM_mb);
}

inline void
to_json(json &j, const HostInfo &h)
{
    j = json{{"name", h.name},
             {"OS", h.OS},
             {"CPUs", h.CPUs},
             {"GPUs", h.GPUs},
             {"RAM_mb", h.RAM_mb}};
}

inline void
from_json(const json &j, HostInfo &h)
{
    j.at("name").get_to(h.name);
    j.at("OS").get_to(h.OS);
    j.at("CPUs").get_to(h.CPUs);
    j.at("GPUs").get_to(h.GPUs);
    j.at("RAM_mb").get_to(h.RAM_mb);
}
