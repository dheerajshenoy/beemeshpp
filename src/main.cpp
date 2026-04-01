#include "Hive.hpp"
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
        .default_value("8080")
        .nargs(1)
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
        .default_value("8080")
        .nargs(1)
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
        .default_value("8080")
        .nargs(1)
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
        .default_value("8080")
        .nargs(1)
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

    if (parser.is_subcommand_used("monitor"))
    {
    }
    else if (parser.is_subcommand_used("launch"))
    {
    }
    else if (parser.is_subcommand_used("hive"))
    {

        std::string auth_token = hive_command.get<std::string>("--auth-token");
        std::string host       = hive_command.get<std::string>("--host");
        std::string port       = hive_command.get<std::string>("--port");

        Hive hive(auth_token, host, port);
        hive.run();
    }
    else if (parser.is_subcommand_used("bee"))
    {
        std::string auth_token = bee_command.get<std::string>("--auth-token");
        std::string host       = bee_command.get<std::string>("--host");
        std::string port       = bee_command.get<std::string>("--port");

        Bee bee(auth_token, host, port);
        bee.run();
    }

    return 0;
}
