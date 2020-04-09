#include <stdexcept>

#include "util.h"
#include "DBusServer.h"

using namespace logid;

DBusServer::DBusServer()
{
    FILE* dbus_fp;
    dbus_fp = fopen(LOGID_LOCAL_DBUS_XML, "r");
    if(!dbus_fp)
    {
        dbus_fp = fopen(LOGID_DBUS_XML_LOCATION, "r");
        if(!dbus_fp)
            std::runtime_error("Could not load DBus XML file.");
    }

    fseek(dbus_fp, 0, SEEK_END);
    dbus_xml.resize(std::ftell(dbus_fp));
    rewind(dbus_fp);
    fread(&dbus_xml[0], 1, dbus_xml.size(), dbus_fp);
    fclose(dbus_fp);
}