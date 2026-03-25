#pragma once

#include <argparse.hpp>
#include <asio.hpp>

class BeeMesh
{
public:
    BeeMesh(const argparse::ArgumentParser &argparser);
    ~BeeMesh();

    enum class Mode
    {
        None = 0,
        Monitor,
        Launch,
        Hive,
        Bee
    };

private:
    void parse_args(const argparse::ArgumentParser &argparser);

    std::string m_port;
    std::string m_host;
    std::string m_hive_url;
    std::string m_auth_token;

    Mode m_mode = Mode::None;
};
