#include "Root.h"
#include "../logid.h"
#include <thread>

using namespace IPC;
using namespace logid;
using namespace pizza::pixl;

Root::Root(DBus::Connection &connection): DBus::ObjectAdaptor(
        connection, "/pizza/pixl/logiops")
{
    Version = LOGIOPS_VERSION;
}

void Root::Reload()
{
    std::thread {[=]() {
        reload();
    } }.detach();
}