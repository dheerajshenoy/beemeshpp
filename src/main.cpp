#include "BeeMesh.hpp"
#include "argparse.hpp"

void
init_args(argparse::ArgumentParser &program)
{
    program.add_argument("-v", "--verbose")
        .help("Increase output verbosity")
        .flag();

    program.add_argument("--monitor")
        .help("Monitor the BeeMesh network and display status information")
        .flag();

    program.add_argument("--launch")
        .help("Launch provided command or executable in the BeeMesh network")
        .nargs(1)
        .metavar("COMMAND|EXECUTABLE");

    program.add_argument("--hive")
        .help("Start BeeMesh in hive mode, allowing other nodes to connect and "
              "execute tasks")
        .flag();

    program.add_argument("--port")
        .help("Port number to listen on when running in hive mode (default: "
              "8080)")
        .nargs(1)
        .metavar("PORT");

    program.add_argument("--host")
        .help("Hostname or IP address to bind to when running in hive mode or "
              "connect to when running in bee mode"
              "(default: 127.0.0.1)")
        .nargs(1)
        .metavar("HOST");

    program.add_argument("--bee")
        .help("Start BeeMesh in bee mode assigned by the hive")
        .flag();

    program.add_argument("--hive-url")
        .help("URL of the hive to connect to when running in bee mode")
        .nargs(1)
        .metavar("URL");

    program.add_argument("--auth-token")
        .help("Authentication token for connecting to the hive")
        .nargs(1)
        .metavar("TOKEN");
}

int
main(int argc, char *argv[])
{
    argparse::ArgumentParser parser("BeeMesh", BEEMESH_VERSION);
    init_args(parser);
    try
    {
        parser.parse_args(argc, argv);
        if (argc == 1)
        {
            std::println("{}", parser.help().str());
        }
    }
    catch (const std::runtime_error &err)
    {
        std::println("error: {}", err.what());
        return 1;
    }

    BeeMesh beemesh(parser);

    return 0;
}
