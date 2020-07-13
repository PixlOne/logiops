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
            connection, "/pizza/pixl/logiops", "pizza.pixl.logiops") {}
};

void logictl::reload()
{
    Root root(*bus);
    std::string logid_version = root.Version();
    if(logid_version != LOGIOPS_VERSION) {
        fprintf(stderr,
                "Error: Version mismatch: logid version is %s, logictl version is %s\n",
                logid_version.c_str(), LOGIOPS_VERSION);
        exit(EXIT_FAILURE);
    }
    root.Reload();
}