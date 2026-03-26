#include "BeeMesh.hpp"
#include "Endpoint.hpp"
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

    argparse::ArgumentParser monitor_command("monitor");
    monitor_command.add_description(
        "Monitor the BeeMesh network and display status information");

    monitor_command.add_argument("--host")
        .help("Hostname or IP address for the monitor to bind to")
        .nargs(1)
        .default_value("localhost")
        .metavar("HOST");

    monitor_command.add_argument("--port")
        .help("Port number for the monitor to listen on")
        .default_value(Port{8080})
        .nargs(1)
        .scan<'u', Port>()
        .metavar("PORT");

    argparse::ArgumentParser launch_command("launch");
    launch_command.add_description("Launch provided command or executable in "
                                   "the BeeMesh network");

    launch_command.add_argument("--host")
        .help("Hostname or IP address for the launch command to connect to the "
              "hive")
        .default_value("localhost")
        .nargs(1)
        .metavar("HOST");

    launch_command.add_argument("--port")
        .help("Port number for the launch command to connect to the hive")
        .default_value(Port{8080})
        .nargs(1)
        .scan<'u', Port>()
        .metavar("PORT");

    launch_command.add_argument("--payload")
        .help("Command or executable to run in the BeeMesh network")
        .default_value("echo 'Hello, BeeMesh!'")
        .nargs(1)
        .metavar("PAYLOAD");

    argparse::ArgumentParser hive_command("hive");
    hive_command.add_description("Start BeeMesh in hive mode, allowing other "
                                 "nodes to connect and "
                                 "execute tasks");

    hive_command.add_argument("--port")
        .help("Port number for the hive to listen on")
        .default_value(Port{8080})
        .nargs(1)
        .scan<'u', Port>()
        .metavar("PORT");

    hive_command.add_argument("--host")
        .help("Hostname or IP address for the hive to bind to")
        .default_value("localhost")
        .nargs(1)
        .metavar("HOST");

    hive_command.add_argument("--auth-token")
        .help("Authentication token for connecting bees to the hive")
        .default_value("default_token")
        .nargs(1)
        .metavar("TOKEN");

    argparse::ArgumentParser bee_command("bee");
    bee_command.add_description(
        "Start BeeMesh in bee mode assigned by the hive");

    bee_command.add_argument("--host")
        .help("Hostname or IP address of the hive to connect to")
        .default_value("localhost")
        .nargs(1)
        .metavar("HOST");

    bee_command.add_argument("--port")
        .help("Port number for the bee to connect to the hive")
        .default_value(Port{8080})
        .nargs(1)
        .scan<'u', Port>()
        .metavar("PORT");

    bee_command.add_argument("--auth-token")
        .help("Authentication token for connecting to the hive")
        .default_value("default_token")
        .nargs(1)
        .metavar("TOKEN");

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

    BeeMesh beemesh;

    if (parser.is_subcommand_used("monitor"))
    {
        beemesh.start_monitor_mode(monitor_command);
    }
    else if (parser.is_subcommand_used("launch"))
    {
        beemesh.start_launch_mode(launch_command);
    }
    else if (parser.is_subcommand_used("hive"))
    {
        beemesh.start_hive_mode(hive_command);
    }
    else if (parser.is_subcommand_used("bee"))
    {
        beemesh.start_bee_mode(bee_command);
    }

    return 0;
}
