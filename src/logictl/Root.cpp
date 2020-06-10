#include "logictl.h"
#include "dbus_client.h"

using namespace logictl;
using namespace pizza::pixl;

class Root : public logiops_proxy,
                public DBus::IntrospectableProxy,
                public DBus::ObjectProxy
{
public:
    Root(DBus::Connection &connection): DBus::ObjectProxy(
            connection, "/", "pizza.pixl.logiops") {}
};

void logictl::reload()
{
    Root root(*bus);
    root.Reload();
}