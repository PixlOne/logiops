#ifndef LOGID_IPC_ROOT_H
#define LOGID_IPC_ROOT_H

#include "dbus_server.h"

namespace logid::IPC
{
    using namespace pizza::pixl;

    class Root : public logiops_adaptor,
                 public DBus::IntrospectableAdaptor,
                 public DBus::PropertiesAdaptor,
                 public DBus::ObjectAdaptor
    {
    public:
        Root(DBus::Connection &connection);
        virtual void Reload();
    };
}

#endif //LOGID_IPC_ROOT_H