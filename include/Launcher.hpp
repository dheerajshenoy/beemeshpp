#pragma once
#include <string>

class Launcher
{
public:
    Launcher(const std::string &host, const std::string &port,
             const std::string &payload);
    void run();

private:
    std::string m_host, m_port, m_payload;
};
