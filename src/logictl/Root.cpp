/*
 * Copyright 2019-2020 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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