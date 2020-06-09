#ifndef LOGID_DBUSSERVER_H
#define LOGID_DBUSSERVER_H

#include "ipc/dbus_server.h"

#include <string>

namespace logid {
    namespace IPC
    {
        using namespace pizza::pixl;

        class Root : public logiops_adaptor,
                        public DBus::IntrospectableAdaptor,
                        public DBus::ObjectAdaptor
        {
        public:
            Root(DBus::Connection &connection): DBus::ObjectAdaptor(
                    connection, "pizza.pixl.logiops") {}

            virtual void Reload();
        };
    }

    class IPCServer
    {
    public:
        IPCServer();
        void start();
        void stop();
    private:
        std::thread* ipc_thread;
        std::string dbus_xml;
        IPC::Root* _root;
        DBus::BusDispatcher* dispatcher;
        DBus::Connection* bus;
    };

    extern IPCServer* ipc_server;
};

#endif // LOGID_DBUSSERVER_H