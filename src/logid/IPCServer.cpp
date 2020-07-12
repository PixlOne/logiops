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

#include <stdexcept>
#include <thread>

#include "logid.h"
#include "util/log.h"
#include "ipc/util.h"
#include "ipc/dbus_server.h"
#include "IPCServer.h"

using namespace logid;
using namespace pizza::pixl;
using namespace pizza::pixl::logiops;

IPCServer::IPCServer()
{
    dispatcher = new DBus::BusDispatcher;

    DBus::default_dispatcher = dispatcher;
    auto _bus = DBus::Connection::SessionBus();
    bus = &_bus;

    bus->request_name("pizza.pixl.logiops");
    _root = new IPC::Root(*bus);

    ipc_thread = nullptr;
}

void IPCServer::start()
{
    ipc_thread = new std::thread {[=]() { dispatcher->enter(); } };
}

void IPCServer::stop()
{
    if(ipc_thread != nullptr)
    {
        dispatcher->leave();
        ipc_thread->join();
    }
}

void IPCServer::addDevice(Device* device)
{
    std::vector<std::string> devices = _root->Devices();
    std::string device_name = IPC::toDevName(device->path, device->index);
    devices.push_back(device_name);
    _root->Devices = devices;
    _devices.insert({device_name, new IPC::Device(_root->conn(), device)});
    _root->DeviceConnected(device_name);
}

void IPCServer::removeDevice(std::string path, HIDPP::DeviceIndex index)
{
    std::vector<std::string> devices = _root->Devices();
    std::string device_name = IPC::toDevName(path, index);
    for(auto it = devices.begin(); it != devices.end(); it++) {
        if(*it == device_name) {
            devices.erase(it);
            break;
        }
    }
    _root->Devices = devices;
    auto it = _devices.find(device_name);
    if(it != _devices.end())
    {
        delete(it->second);
        _devices.erase(it);
    }
    _root->DeviceDisconnected(device_name);
}

void IPCServer::addReceiver(std::string path)
{
    std::vector<std::string> receivers = _root->Receivers();
    receivers.push_back(path);
    _root->Receivers = receivers;
    _root->ReceiverConnected(path);
}

void IPCServer::removeReceiver(std::string path)
{
    std::vector<std::string> receivers = _root->Receivers();
    for(auto it = receivers.begin(); it != receivers.end(); it++) {
        if(*it == path) {
            receivers.erase(it);
            break;
        }
    }
    _root->Receivers = receivers;
    _root->ReceiverDisconnected(path);
}