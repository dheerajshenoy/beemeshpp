#pragma once

#include "HostInfo.hpp"
#include "MessageType.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <vector>

#if defined(__APPLE__)
    #include <sys/sysctl.h>
#endif

#if !defined(_WIN32)
    #include <unistd.h>
#endif

namespace Utils
{

    static std::string get_os_name()
    {
#if defined(_WIN32)
        return "Windows";
#elif defined(_WIN64)
        return "Windows 64-bit";
#elif defined(__APPLE__) || defined(__MACH__)
        return "macOS";
#elif defined(__linux__)
        return "Linux";
#elif defined(__unix__)
        return "Unix";
#else
        return "Unknown OS";
#endif
    }

    static std::string get_host_name()
    {
        char hostname[256] = {0};

#if defined(_WIN32)
        DWORD size = sizeof(hostname);
        if (!GetComputerNameA(hostname, &size))
            std::cerr << "Failed to get device name\n";
#else
        if (!(gethostname(hostname, sizeof(hostname)) == 0))
            std::cerr << "Failed to get device name\n";
#endif
        return hostname;
    }

    inline std::string get_arch()
    {
#if defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
        return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
        return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
        return "arm";
#elif defined(__riscv)
        return "riscv";
#else
        return "unknown";
#endif
    }

    inline std::vector<HostInfo::CPU> get_cpus()
    {
        std::vector<HostInfo::CPU> cpus;

        HostInfo::CPU cpu;
        cpu.arch      = get_arch();
        cpu.n_threads = std::thread::hardware_concurrency();
        cpu.n_cores   = cpu.n_threads; // fallback if we can't determine
        cpu.name      = "Unknown";

#if defined(__linux__)
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f)
        {
            char line[512];
            // Track unique (physical_id, core_id) pairs to count physical cores
            std::set<std::pair<int, int>> seen;
            int cur_physical = 0, cur_core = -1;
            bool has_topology = false;

            while (fgets(line, sizeof(line), f))
            {
                auto trim = [](std::string s) {
                    auto e = s.find_last_not_of(" \t\r\n");
                    return e == std::string::npos ? "" : s.substr(0, e + 1);
                };

                if (strncmp(line, "model name", 10) == 0 && cpu.name == "Unknown")
                {
                    char *colon = strchr(line, ':');
                    if (colon)
                        cpu.name = trim(std::string(colon + 2));
                }
                else if (strncmp(line, "physical id", 11) == 0)
                {
                    char *colon = strchr(line, ':');
                    if (colon) { cur_physical = atoi(colon + 2); has_topology = true; }
                }
                else if (strncmp(line, "core id", 7) == 0)
                {
                    char *colon = strchr(line, ':');
                    if (colon) cur_core = atoi(colon + 2);
                }
                else if (line[0] == '\n' && cur_core >= 0 && has_topology)
                {
                    seen.insert({cur_physical, cur_core});
                    cur_core = -1;
                }
            }
            if (cur_core >= 0 && has_topology)
                seen.insert({cur_physical, cur_core});
            if (!seen.empty())
                cpu.n_cores = static_cast<unsigned>(seen.size());

            fclose(f);
        }

#elif defined(__APPLE__)
        {
            char brand[256] = {};
            size_t len      = sizeof(brand);
            if (sysctlbyname("machdep.cpu.brand_string", brand, &len, nullptr, 0) == 0)
                cpu.name = brand;

            int val = 0;
            len     = sizeof(val);
            if (sysctlbyname("hw.physicalcpu", &val, &len, nullptr, 0) == 0)
                cpu.n_cores = static_cast<unsigned>(val);
            len = sizeof(val);
            if (sysctlbyname("hw.logicalcpu", &val, &len, nullptr, 0) == 0)
                cpu.n_threads = static_cast<unsigned>(val);
        }

#elif defined(_WIN32)
        cpu.name = "Windows CPU";
#endif

        cpus.push_back(cpu);
        return cpus;
    }

    inline std::vector<HostInfo::GPU> get_gpus()
    {
        std::vector<HostInfo::GPU> gpus;

#if defined(__linux__)
        // Walk /sys/class/drm/card* entries; each represents a GPU.
        for (int i = 0;; ++i)
        {
            std::string base =
                "/sys/class/drm/card" + std::to_string(i) + "/device/";

            std::ifstream vendor_f(base + "vendor");
            if (!vendor_f.good())
                break;

            std::string vendor_hex;
            vendor_f >> vendor_hex;

            HostInfo::GPU gpu;
            gpu.VRAM_mb = 0;

            if (vendor_hex == "0x10de")
            {
                gpu.vendor = "NVIDIA";
                // Try to get the model name and VRAM from the NVIDIA proc interface
                for (int g = 0; g < 8; ++g)
                {
                    std::string info_path = "/proc/driver/nvidia/gpus/"
                                           + std::to_string(g) + "/information";
                    std::ifstream info(info_path);
                    if (!info.good())
                        break;
                    std::string line;
                    while (std::getline(info, line))
                    {
                        if (line.rfind("Model:", 0) == 0)
                            gpu.name = line.substr(line.find(':') + 2);
                        else if (line.rfind("Video Memory:", 0) == 0)
                        {
                            // "Video Memory:      24564 MB"
                            auto pos = line.find_last_of(' ');
                            auto mpos = line.rfind("MB");
                            if (mpos != std::string::npos)
                                gpu.VRAM_mb = static_cast<unsigned>(
                                    std::stoul(line.substr(
                                        line.rfind(' ', mpos - 2) + 1)));
                        }
                    }
                    if (!gpu.name.empty())
                        break;
                }
                if (gpu.name.empty())
                    gpu.name = "NVIDIA GPU";
            }
            else if (vendor_hex == "0x1002")
            {
                gpu.vendor = "AMD";
                gpu.name   = "AMD GPU";
                std::ifstream vram_f(base + "mem_info_vram_total");
                if (vram_f.good())
                {
                    uint64_t bytes = 0;
                    vram_f >> bytes;
                    gpu.VRAM_mb = static_cast<unsigned>(bytes / (1024 * 1024));
                }
            }
            else if (vendor_hex == "0x8086")
            {
                gpu.vendor = "Intel";
                gpu.name   = "Intel GPU";
            }
            else
            {
                gpu.vendor = vendor_hex;
                gpu.name   = "Unknown GPU";
            }

            gpus.push_back(gpu);
        }

#elif defined(_WIN32)
        IDXGIFactory *factory = nullptr;
        if (SUCCEEDED(
                CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory)))
        {
            IDXGIAdapter *adapter = nullptr;
            for (UINT i = 0;
                 factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND;
                 ++i)
            {
                DXGI_ADAPTER_DESC desc;
                adapter->GetDesc(&desc);
                HostInfo::GPU gpu;
                std::wstring wname(desc.Description);
                gpu.name    = std::string(wname.begin(), wname.end());
                gpu.VRAM_mb = static_cast<unsigned>(
                    desc.DedicatedVideoMemory / (1024 * 1024));
                gpus.push_back(gpu);
                adapter->Release();
            }
            factory->Release();
        }
#endif

        if (gpus.empty())
        {
            HostInfo::GPU gpu;
            gpu.name    = "Unknown";
            gpu.vendor  = "Unknown";
            gpu.VRAM_mb = 0;
            gpus.push_back(gpu);
        }

        return gpus;
    }

    inline unsigned get_host_total_ram_mb()
    {
#if defined(_WIN32)
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return static_cast<unsigned>(status.ullTotalPhys / (1024 * 1024));
#elif defined(__APPLE__)
        int64_t ram;
        size_t len = sizeof(ram);
        sysctlbyname("hw.memsize", &ram, &len, nullptr, 0);
        return static_cast<unsigned>(ram / (1024 * 1024));
#elif defined(__linux__)
        long pages     = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGESIZE);
        return static_cast<unsigned>((pages * page_size) / (1024 * 1024));
#else
        return 0;
#endif
    }

    inline HostInfo get_host_info()
    {
        return HostInfo{.name   = Utils::get_host_name(),
                        .OS     = Utils::get_os_name(),
                        .CPUs   = Utils::get_cpus(),
                        .GPUs   = Utils::get_gpus(),
                        .RAM_mb = Utils::get_host_total_ram_mb()};
    }

    // This has to be kept in sync with the MessageType enum in MessageType.hpp
    inline constexpr std::array<std::string_view, (int)MessageType::COUNT>
        MESSAGE_TYPE_STRINGS
        = {"bee_registration", "bee_id_assignment", "job_assignment",
           "job_result",       "status_update",     "status_check",
           "error_report",     "job_submission",    "monitor_connect",
           "status_snapshot"};

    inline std::string_view to_string(MessageType type)
    {
        return MESSAGE_TYPE_STRINGS[static_cast<std::size_t>(type)];
    }

    inline MessageType message_type_from_string(std::string_view s)
    {
        const auto it = std::ranges::find(MESSAGE_TYPE_STRINGS, s);
        if (it == MESSAGE_TYPE_STRINGS.end())
            throw std::invalid_argument(std::string("Unknown MessageType: ")
                                        + std::string(s));
        return static_cast<MessageType>(
            std::distance(MESSAGE_TYPE_STRINGS.begin(), it));
    }

}; // namespace Utils
