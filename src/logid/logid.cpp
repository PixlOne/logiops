#include <hidpp/SimpleDispatcher.h>
#include <hidpp/DispatcherThread.h>
#include <hidpp20/Device.h>
#include <hidpp20/Error.h>
#include <hidpp20/IReprogControls.h>
#include <hidpp20/UnsupportedFeature.h>
#include <hid/DeviceMonitor.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "util.h"
#include "Device.h"
#include "Actions.h"
#include "Configuration.h"
#include "EvdevDevice.h"
#include "DeviceFinder.h"

#define evdev_name "logid"
#define DEFAULT_CONFIG_FILE "/etc/logid.cfg"

#ifndef LOGIOPS_VERSION
#define LOGIOPS_VERSION "null"
#warning Version is undefined!
#endif

using namespace logid;

LogLevel logid::global_verbosity = INFO;
Configuration* logid::global_config;
EvdevDevice* logid::global_evdev;
DeviceFinder* logid::finder;

enum class Option
{
    None,
    Verbose,
    Config,
    Help,
    Version
};

int main(int argc, char** argv)
{
    std::string config_file = DEFAULT_CONFIG_FILE;
    // Read command line options
    for(int i = 1; i < argc; i++)
    {
        Option option = Option::None;
        if(argv[i][0] == '-') // This argument is an option
        {
            switch(argv[i][1]) // Set option
            {
                case '-': // Full option name
                {
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
                    log_printf(WARN, "%s is not a valid option, ignoring.", argv[1]);
            }
            switch(option)
            {
                case Option::Verbose:
                {
                    if (++i >= argc)
                    {
                        global_verbosity = DEBUG; // Assume debug verbosity
                        break;
                    }
                    std::string loglevel = argv[i];
                    try { global_verbosity = stringToLogLevel(argv[i]); }
                    catch (std::invalid_argument &e)
                    {
                        if (argv[i][0] == '-')
                        {
                            global_verbosity = DEBUG; // Assume debug verbosity
                            i--; // Go back to last argument to continue loop.
                        }
                        else
                        {
                            log_printf(WARN, e.what());
                            printf("Valid verbosity levels are: Debug, Info, Warn/Warning, or Error.\n");
                            return EXIT_FAILURE;
                        }
                    }
                    break;
                }
                case Option::Config:
                {
                    if (++i >= argc)
                    {
                        log_printf(ERROR, "Config file is not specified.");
                        return EXIT_FAILURE;
                    }
                    config_file = argv[i];
                    break;
                }
                case Option::Help:
                {
                    printf(R"(logid version %s
Usage: %s [options]
Possible options are:
    -v,--verbose [level]       Set log level to debug/info/warn/error (leave blank for debug)
    -V,--version               Print version number
    -c,--config [file path]    Change config file from default at %s
    -h,--help                  Print this message.
)", LOGIOPS_VERSION, argv[0], DEFAULT_CONFIG_FILE);

                    return EXIT_SUCCESS;
                }
                case Option::Version:
                {
                    printf("%s\n", LOGIOPS_VERSION);
                    return EXIT_SUCCESS;
                }
                case Option::None:
                    break;
            }
        }
    }

    // Read config
    try { global_config = new Configuration(config_file.c_str()); }
    catch (std::exception &e) { global_config = new Configuration(); }

    //Create an evdev device called 'logid'
    try { global_evdev = new EvdevDevice(evdev_name); }
    catch(std::system_error& e)
    {
        log_printf(ERROR, "Could not create evdev device: %s", e.what());
        return EXIT_FAILURE;
    }

    // Scan devices, create listeners, handlers, etc.
    finder = new DeviceFinder();
    finder->run();

    return EXIT_SUCCESS;
}