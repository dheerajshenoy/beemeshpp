#pragma once

#include "HostInfo.hpp"

#include <string>

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
        if (!gethostname(hostname, sizeof(hostname)) == 0)
            std::cerr << "Failed to get device name\n";
#endif
        return hostname;
    }

    inline std::string get_arch()
    {
        return "TODO";
    }

    inline std::vector<HostInfo::CPU> get_cpus()
    {
        std::vector<HostInfo::CPU> cpus;

        HostInfo::CPU cpu;
        cpu.n_cores = std::thread::hardware_concurrency();
        cpu.name    = "Unknown";
        cpu.arch    = "Unknown";

#if defined(__linux__)
        // Try reading /proc/cpuinfo for model name
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f)
        {
            char line[256];
            while (fgets(line, sizeof(line), f))
            {
                if (strncmp(line, "model name", 10) == 0)
                {
                    char *colon = strchr(line, ':');
                    if (colon)
                    {
                        cpu.name = std::string(colon + 2); // skip ": "
                        cpu.name.erase(cpu.name.find_last_not_of("\n")
                                       + 1); // trim newline
                    }
                    break;
                }
            }
            fclose(f);
        }
#elif defined(_WIN32)
        // On Windows you’d use CPUID or registry (more code), skip for minimal
        cpu.name = "Windows CPU";
#elif defined(__APPLE__)
        cpu.name = "Apple CPU";
#endif
        cpus.push_back(cpu);
        return cpus;
    }

    inline std::vector<HostInfo::GPU> get_gpus()
    {
        std::vector<HostInfo::GPU> gpus;

#if defined(_WIN32)
        // Minimal Windows DirectX 11 query
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
            gpu.name = std::wstring(desc.Description).begin(), std::wstring(desc.Description).end());
            gpu.vramMB = static_cast<unsigned>(desc.DedicatedVideoMemory
                                               / (1024 * 1024));
            gpus.push_back(gpu);
            }
            factory->Release();
        }
#else
        // On Linux/macOS, you need OpenGL/GLFW/SDL to query GPU
        HostInfo::GPU gpu;
        gpu.name    = "Unknown GPU";
        gpu.VRAM_mb = 0;
        gpus.push_back(gpu);
#endif

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

    HostInfo get_host_info()
    {
        return HostInfo{.name   = Utils::get_host_name(),
                        .OS     = Utils::get_os_name(),
                        .CPUs   = Utils::get_cpus(),
                        .GPUs   = Utils::get_gpus(),
                        .RAM_mb = Utils::get_host_total_ram_mb()};
    }

}; // namespace Utils
