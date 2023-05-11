/*
 * Copyright 2019-2023 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <DeviceManager.h>
#include <InputDevice.h>
#include <util/task.h>
#include <util/log.h>
#include <algorithm>
#include <ipc_defs.h>

#ifndef LOGIOPS_VERSION
#define LOGIOPS_VERSION "null"
#warning Version is undefined!
#endif

static constexpr auto virtual_input_name = "LogiOps Virtual Input";
static constexpr auto default_config = "/etc/logid.cfg";

using namespace logid;

struct CmdlineOptions {
    std::string config_file = default_config;
};

LogLevel logid::global_loglevel = INFO;

enum class Option {
    None,
    Verbose,
    Config,
    Help,
    Version
};

void readCliOptions(const int argc, char** argv, CmdlineOptions& options) {
    for (int i = 1; i < argc; i++) {
        Option option = Option::None;
        if (argv[i][0] == '-') {
            // This argument is an option
            switch (argv[i][1]) {
                case '-': {
                    // Full option name
                    std::string op_str = argv[i];
                    if (op_str == "--verbose") option = Option::Verbose;
                    if (op_str == "--config") option = Option::Config;
                    if (op_str == "--help") option = Option::Help;
                    if (op_str == "--version") option = Option::Version;
                    break;
                }
                case 'v': // Verbosity
                    option = Option::Verbose;
                    break;
                case 'V': //Version
                    option = Option::Version;
                    break;
                case 'c': // Config file path
                    option = Option::Config;
                    break;
                case 'h': // Help
                    option = Option::Help;
                    break;
                default:
                    logPrintf(WARN, "%s is not a valid option, ignoring.",
                              argv[i]);
            }

            switch (option) {
                case Option::Verbose: {
                    if (++i >= argc) {
                        global_loglevel = DEBUG; // Assume debug verbosity
                        break;
                    }
                    std::string loglevel = argv[i];
                    try {
                        global_loglevel = toLogLevel(argv[i]);
                    } catch (std::invalid_argument& e) {
                        if (argv[i][0] == '-') {
                            global_loglevel = DEBUG; // Assume debug verbosity
                            i--; // Go back to last argument to continue loop.
                        } else {
                            logPrintf(WARN, e.what());
                            printf("Valid verbosity levels are: Debug, Info, "
                                   "Warn/Warning, or Error.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    break;
                }
                case Option::Config: {
                    if (++i >= argc) {
                        logPrintf(ERROR, "Config file is not specified.");
                        exit(EXIT_FAILURE);
                    }
                    options.config_file = argv[i];
                    break;
                }
                case Option::Help:
                    printf(R"(logid version %s
Usage: %s [options]
Possible options are:
    -v,--verbose [level]       Set log level to debug/info/warn/error (leave blank for debug)
    -V,--version               Print version number
    -c,--config [file path]    Change config file from default at %s
    -h,--help                  Print this message.
)", LOGIOPS_VERSION, argv[0], default_config);
                    exit(EXIT_SUCCESS);
                case Option::Version:
                    printf("%s\n", LOGIOPS_VERSION);
                    exit(EXIT_SUCCESS);
                case Option::None:
                    break;
            }
        }
    }
}

int main(int argc, char** argv) {
    CmdlineOptions options{};
    readCliOptions(argc, argv, options);
    std::shared_ptr<Configuration> config;
    std::shared_ptr<InputDevice> virtual_input;


    /* Set stdout buff to Null so that loging system like journal
     * can actually read it.
     */
    setbuf(stdout, NULL);

    // Read config
    try {
        config = std::make_shared<Configuration>(options.config_file);
    } catch (std::exception &e) {
        logPrintf(ERROR, "%s", e.what());
        return EXIT_FAILURE;
    }

    init_workers(config->workers.value_or(defaults::workers));

#ifdef USE_USER_BUS
    auto server_bus = ipcgull::IPCGULL_USER;
#else
    auto server_bus = ipcgull::IPCGULL_SYSTEM;
#endif

    auto server = ipcgull::make_server(SERVICE_ROOT_NAME, server_root_node, server_bus);

    //Create a virtual input device
    try {
        virtual_input = std::make_unique<InputDevice>(virtual_input_name);
    } catch (std::system_error& e) {
        logPrintf(ERROR, "Could not create input device: %s", e.what());
        return EXIT_FAILURE;
    }

    // Device manager runs on its own I/O thread asynchronously
    auto device_manager = DeviceManager::make<DeviceManager>(config, virtual_input, server);

    device_manager->enumerate();

    try {
        server->start();
    } catch (ipcgull::connection_failed& e) {
        logPrintf(ERROR, "Lost IPC connection, terminating.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
