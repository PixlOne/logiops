#include "logictl.h"

using namespace logictl;

DBus::BusDispatcher* logictl::dispatcher;
DBus::Connection* logictl::bus;

std::string logictl::exec_cmd;
std::vector<std::string> logictl::args;

void logictl::help()
{
    printf(
R"(Usage: %s [command] [args]
Commands:
    help        Show this dialog
    reload      Reloads logid config and rescans devices
)", exec_cmd.c_str());
}

int main(int argc, char** argv)
{
    dispatcher = new DBus::BusDispatcher();
    DBus::default_dispatcher = dispatcher;
    auto _bus = DBus::Connection::SessionBus();
    bus = &_bus;

    exec_cmd = argv[0];

    for(int i = 1; i < argc; i++)
        logictl::args.emplace_back(argv[i]);

    if(!args.empty())
    {
        auto function = functions.find(args[0]);
        if(function != functions.end())
            function->second();
        else
        {
            printf("%s is not a valid command. Run %s help for a list of valid commands\n",
                    args[0].c_str(), exec_cmd.c_str());
            return 1;
        }
    }
    else
        help();

    return 0;
}