#include <stdexcept>

#include "logid.h"
#include "util.h"
#include "ipc/dbus_server.h"
#include "IPCServer.h"

using namespace logid;
using namespace io::github::pixlone::LogiOps;

void IPC::Control::Reload()
{
    std::thread {[=]() {
        reload();
    } }.detach();
}

IPCServer::IPCServer()
{
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