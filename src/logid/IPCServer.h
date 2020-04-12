#ifndef LOGID_DBUSSERVER_H
#define LOGID_DBUSSERVER_H

#define LOGID_LOCAL_DBUS_XML "./dbus/logid.xml"

#ifndef LOGID_DBUS_XML_LOCATION
#error "logid DBus XML location not defined"
#endif

#include "ipc/dbus_server.h"

#include <string>

namespace logid {
    namespace IPC
    {
        using namespace io::github::pixlone::LogiOps;

        class Control : public Control_adaptor,
                        public DBus::IntrospectableAdaptor,
                        public DBus::ObjectAdaptor
        {
        public:
            Control(DBus::Connection &connection): DBus::ObjectAdaptor(
                    connection, "io.github.pixlone.LogiOps.Control") {}

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
        IPC::Control* _control;
        DBus::BusDispatcher* dispatcher;
        DBus::Connection* bus;
    };

    extern IPCServer* ipc_server;
};

#endif // LOGID_DBUSSERVER_H