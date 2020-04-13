#ifndef LIBLOGIOPS_CLIENT_CONTROL_H
#define LIBLOGIOPS_CLIENT_CONTROL_H

#include "dbus_client.h"

namespace LogiOpsClient
{
    using namespace io::github::pixlone::LogiOps;

    class Control : public Control_proxy,
                    public DBus::IntrospectableProxy,
                    public DBus::ObjectProxy
    {
    public:
        Control(DBus::Connection &connection): DBus::ObjectProxy(
                connection, "/", "io.github.pixlone.LogiOps.Control") {}
    };
}


#endif //LIBLOGIOPS_CLIENT_CONTROL_H
