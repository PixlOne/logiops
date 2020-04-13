#include <liblogiops_client/Control.h>
#include <stdio.h>
#include <cstring>

using namespace LogiOpsClient;

int main(int argc, char** argv)
{
    DBus::BusDispatcher dispatcher;

    DBus::default_dispatcher = &dispatcher;
    auto bus = DBus::Connection::SessionBus();
    bus.request_name("io.github.pixlone.LogiOps.Control");

    if(argc >= 1)
    {
        if(!strcmp(argv[1], "reload"))
        {
            Control control(bus);
            control.Reload();
            return 0;
        }
        else
            printf("Unknown command: %s", argv[1]);
    }

    return 0;
}