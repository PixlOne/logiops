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