#include "Root.h"
#include "../logid.h"
#include <thread>

using namespace logid::IPC;
using namespace logid;
using namespace pizza::pixl;

Root::Root(DBus::Connection &connection): DBus::ObjectAdaptor(
        connection, "/pizza/pixl/logiops")
{
    Version = LOGIOPS_VERSION;
    std::vector<std::string> _dev;
    std::vector<std::string> _recv;
    Devices = _dev;
    Receivers = _recv;
}

void Root::Reload()
{
    std::thread {[=]() {
        reload();
    } }.detach();
}