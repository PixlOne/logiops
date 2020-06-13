#ifndef LOGID_IPC_DEVICE_H
#define LOGID_IPC_DEVICE_H

#include "../Device.h"
#include "dbus_server.h"

namespace logid::IPC
{
    using namespace pizza::pixl::logiops;

    class Device : public Device_adaptor,
                 public DBus::IntrospectableAdaptor,
                 public DBus::PropertiesAdaptor,
                 public DBus::ObjectAdaptor
    {
    public:
        Device(DBus::Connection &connection, logid::Device* device);
        virtual void GetInfo(const std::string& device);
    private:
        logid::Device* _device;
    };
}

#endif //LOGID_IPC_UTIL_H