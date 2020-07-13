#ifndef LOGID_DBUSSERVER_H
#define LOGID_DBUSSERVER_H

#include "ipc/dbus_server.h"
#include "ipc/Root.h"
#include "ipc/Device.h"

#include <string>

namespace logid {
    class IPCServer
    {
    public:
        IPCServer();
        void start();
        void stop();
        void addDevice(Device* device);
        void removeDevice(std::string path, HIDPP::DeviceIndex index);
        void addReceiver(std::string path);
        void removeReceiver(std::string path);
    private:
        std::thread* ipc_thread;
        std::string dbus_xml;
        IPC::Root* _root;
        std::map<std::string, IPC::Device*> _devices;
        DBus::BusDispatcher* dispatcher;
        DBus::Connection* bus;
    };

    extern IPCServer* ipc_server;
};

#endif // LOGID_DBUSSERVER_H