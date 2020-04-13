#include "logictl.h"
#include "dbus_client.h"

using namespace logictl;
using namespace io::github::pixlone::LogiOps;

class Control : public Control_proxy,
                public DBus::IntrospectableProxy,
                public DBus::ObjectProxy
{
public:
    Control(DBus::Connection &connection): DBus::ObjectProxy(
            connection, "/", "io.github.pixlone.LogiOps.Control") {}
};

void logictl::reload()
{
    Control control(*bus);
    control.Reload();
}