#include <stdexcept>
#include <thread>

#include "logid.h"
#include "util.h"
#include "ipc/dbus_server.h"
#include "IPCServer.h"

using namespace logid;
using namespace pizza::pixl;
using namespace pizza::pixl::logiops;

IPCServer::IPCServer()
{
    dispatcher = new DBus::BusDispatcher;

    DBus::default_dispatcher = dispatcher;
    auto _bus = DBus::Connection::SessionBus();
    bus = &_bus;

    bus->request_name("pizza.pixl.logiops");
    _root = new IPC::Root(*bus);

    ipc_thread = nullptr;
}

void IPCServer::start()
{
    ipc_thread = new std::thread {[=]() { dispatcher->enter(); } };
}

void IPCServer::stop()
{
    if(ipc_thread != nullptr)
    {
        dispatcher->leave();
        ipc_thread->join();
    }
}