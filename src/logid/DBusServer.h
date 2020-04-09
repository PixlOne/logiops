#ifndef LOGID_DBUSSERVER_H
#define LOGID_DBUSSERVER_H

#define LOGID_LOCAL_DBUS_XML "./dbus/logid.xml"

#ifndef LOGID_DBUS_XML_LOCATION
#error "logid DBus XML location not defined"
#endif

#include <string>

namespace logid {
    class DBusServer
    {
    public:
        DBusServer();
    private:
         std::string dbus_xml;
    };
};

#endif // LOGID_DBUSSERVER_H