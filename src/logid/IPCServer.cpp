#include <stdexcept>

#include "util.h"
#include "ipc/dbus_server.h"
#include "IPCServer.h"

using namespace logid;
using namespace io::github::pixlone::LogiOps;

void IPC::Control::Reload()
{
    log_printf(INFO, "Reloaded logid!");
}

IPCServer::IPCServer()
{
    FILE* dbus_fp;
    dbus_fp = fopen(LOGID_LOCAL_DBUS_XML, "r");
    if(!dbus_fp)
    {
        dbus_fp = fopen(LOGID_DBUS_XML_LOCATION, "r");
        if(!dbus_fp)
            throw std::runtime_error("Could not load DBus XML file. Check if "
            LOGID_LOCAL_DBUS_XML " or " LOGID_DBUS_XML_LOCATION " exists");
    }

    fseek(dbus_fp, 0, SEEK_END);
    dbus_xml.resize(std::ftell(dbus_fp));
    rewind(dbus_fp);
    fread(&dbus_xml[0], 1, dbus_xml.size(), dbus_fp);
    fclose(dbus_fp);

    dispatcher = new DBus::BusDispatcher;

    DBus::default_dispatcher = dispatcher;
    auto _bus = DBus::Connection::SessionBus();
    bus = &_bus;

    bus->request_name("io.github.pixlone.LogiOps.Control");
    _control = new IPC::Control(*bus);
}

void IPCServer::start()
{
    ipc_thread = new std::thread {[=]() { dispatcher->enter(); } };
}

void IPCServer::stop()
{
    dispatcher->leave();
    ipc_thread->join();
}