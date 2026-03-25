#include "BeeMesh.hpp"

#include "Hive.hpp"

#include <print>

const char *BANNER_STR = R"(
______            ___  ___          _
| ___ \           |  \/  |         | |
| |_/ / ___  ___  | .  . | ___  ___| |__
| ___ \/ _ \/ _ \ | |\/| |/ _ \/ __| '_ \
| |_/ /  __/  __/ | |  | |  __/\__ \ | | |
\____/ \___|\___| \_|  |_/\___||___/_| |_|
    _
   /_/_      .'''.      .''.   ..   .
=O(_)))) ...'     `.  .'    '.'  '.'
   \_\              ``

BeeMesh - Distributed Volunteer Computing Framework
)";

BeeMesh::BeeMesh(const argparse::ArgumentParser &argparser)
{
    parse_args(argparser);

    run();
}

BeeMesh::~BeeMesh() {}

void
BeeMesh::parse_args(const argparse::ArgumentParser &argparser)
{
    if (argparser.is_subcommand_used("monitor"))
    {
        m_mode = Mode::Monitor;
    }

    else if (argparser.is_subcommand_used("launch"))
    {
        m_mode = Mode::Launch;
    }

    else if (argparser.is_subcommand_used("hive"))
    {
        m_mode = Mode::Hive;
    }

    else if (argparser.is_subcommand_used("bee"))
    {
        m_mode = Mode::Bee;
    }

    if (m_mode == Mode::None)
    {
        std::string info_str
            = "No mode specified. Use --monitor, --launch, --hive, or --bee.";
        std::println("{}", info_str);
    }

    if (argparser.is_used("--port"))
    {
        m_port = argparser.get<uint16_t>("--port");
    }

    if (argparser.is_used("--host"))
    {
        m_host = argparser.get<std::string>("--host");
    }

    if (argparser.is_used("--auth-token"))
    {
        m_auth_token = argparser.get<std::string>("--auth-token");
    }
}

void
BeeMesh::run()
{
    std::println("{}", BANNER_STR);

    switch (m_mode)
    {
        case Mode::Hive:
            start_hive_mode();
            break;

        case Mode::Monitor:
            start_monitor_mode();
            break;

        case Mode::Launch:
            start_launch_mode();
            break;

        case Mode::Bee:
            start_bee_mode();
            break;

        default:
            break;
    }
}

void
BeeMesh::start_hive_mode()
{
    Hive hive(m_auth_token, {m_host, m_port});
    hive.run();
}

void
BeeMesh::start_bee_mode()
{
}

void
BeeMesh::start_monitor_mode()
{
}

void
BeeMesh::start_launch_mode()
{
}
