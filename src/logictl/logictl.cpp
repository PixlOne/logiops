#include <cstdio>
#include <cstring>

#include "logictl.h"

using namespace logictl;

DBus::BusDispatcher* logictl::dispatcher;
DBus::Connection* logictl::bus;
std::vector<std::string> logictl::args;

int main(int argc, char** argv)
{
    dispatcher = new DBus::BusDispatcher();
    DBus::default_dispatcher = dispatcher;
    auto _bus = DBus::Connection::SessionBus();
    bus = &_bus;

    for(int i = 1; i < argc; i++)
        logictl::args.emplace_back(argv[i]);

    if(!args.empty())
    {
        auto function = functions.find(args[0]);
        if(function != functions.end())
            function->second();
    }

    return 0;
}