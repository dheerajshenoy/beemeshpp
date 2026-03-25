#include "BeeMesh.hpp"
#include "argparse.hpp"

#include <print>

int
main(int argc, char *argv[])
{
    // Create the main argument parser
    argparse::ArgumentParser parser("BeeMesh", BEEMESH_VERSION);

    parser.add_argument("-v", "--verbose")
        .help("Increase output verbosity")
        .flag();

    parser.add_argument("--port")
        .help("Port number to listen on when running in hive mode (default: "
              "8080) or connect to when running in bee mode")
        .nargs(1)
        .scan<'d', uint16_t>()
        .metavar("PORT");

    parser.add_argument("--host")
        .help("Hostname or IP address to bind to when running in hive mode or "
              "connect to when running in bee mode"
              "(default: 127.0.0.1)")
        .nargs(1)
        .metavar("HOST");

    parser.add_argument("--auth-token")
        .help("Authentication token for connecting to the hive")
        .nargs(1)
        .metavar("TOKEN");

    argparse::ArgumentParser monitor_command("monitor");
    monitor_command.add_description(
        "Monitor the BeeMesh network and display status information");

    argparse::ArgumentParser launch_command("launch");
    launch_command.add_description("Launch provided command or executable in "
                                   "the BeeMesh network");

    argparse::ArgumentParser hive_command("hive");
    hive_command.add_description("Start BeeMesh in hive mode, allowing other "
                                 "nodes to connect and "
                                 "execute tasks");

    argparse::ArgumentParser bee_command("bee");
    bee_command.add_description(
        "Start BeeMesh in bee mode assigned by the hive");

    parser.add_subparser(monitor_command);
    parser.add_subparser(launch_command);
    parser.add_subparser(hive_command);
    parser.add_subparser(bee_command);

    // Parse command-line arguments
    try
    {
        parser.parse_args(argc, argv);
        if (argc == 1)
        {
            std::println("{}", parser.help().str());
            return 0;
        }
    }
    catch (const std::exception &err)
    {
        std::println("error: {}", err.what());
        return 1;
    }

    BeeMesh beemesh(parser);

    return 0;
}
