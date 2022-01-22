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

#include "Receiver.h"
#include "DeviceManager.h"
#include "util/log.h"
#include "backend/hidpp10/Error.h"
#include "backend/hidpp20/Error.h"
#include "backend/Error.h"

using namespace logid;
using namespace logid::backend;

ReceiverNickname::ReceiverNickname(
        const std::shared_ptr<DeviceManager>& manager) :
        _nickname (manager->newReceiverNickname()), _manager (manager)
{
}

ReceiverNickname::operator std::string() const {
    return std::to_string(_nickname);
}

ReceiverNickname::~ReceiverNickname()
{
    if(auto manager = _manager.lock()) {
        std::lock_guard<std::mutex> lock(manager->_nick_lock);
        manager->_receiver_nicknames.erase(_nickname);
    }
}

namespace logid {
    class _Receiver : public Receiver {
    public:
        template <typename... Args>
        _Receiver(Args... args) : Receiver(std::forward<Args>(args)...) { }
    };
}

std::shared_ptr<Receiver> Receiver::make(
        const std::string &path,
        const std::shared_ptr<DeviceManager> &manager) {
    auto ret = std::make_shared<_Receiver>(path, manager);
    ret->_self = ret;
    ret->_ipc_node->manage(ret);
    return ret;
}


Receiver::Receiver(const std::string& path,
                   const std::shared_ptr<DeviceManager>& manager) :
    dj::ReceiverMonitor(path,
                        manager->config()->io_timeout.value_or(
                                defaults::io_timeout)),
    _path (path), _manager (manager), _nickname (manager),
    _ipc_node (manager->receiversNode()->make_child(_nickname)),
    _ipc_interface (_ipc_node->make_interface<ReceiverIPC>(this))
{
}

const Receiver::DeviceList& Receiver::devices() const {
    return _devices;
}

Receiver::~Receiver()
{
    if(auto manager = _manager.lock()) {
        for(auto& d : _devices)
            manager->removeExternalDevice(d.second);
    }
}

void Receiver::addDevice(hidpp::DeviceConnectionEvent event)
{
    std::unique_lock<std::mutex> lock(_devices_change);

    auto manager = _manager.lock();
    if(!manager) {
        logPrintf(ERROR, "Orphan Receiver, missing DeviceManager");
        logPrintf(ERROR,
                  "Fatal error, file a bug report. Program will now exit.");
        std::terminate();
    }

    try {
        // Check if device is ignored before continuing
        if(manager->config()->ignore.value_or(
                std::set<uint16_t>()).contains(event.pid)) {
            logPrintf(DEBUG, "%s:%d: Device 0x%04x ignored.",
                      _path.c_str(), event.index, event.pid);
            return;
        }

        auto dev = _devices.find(event.index);
        if(dev != _devices.end()) {
            if(event.linkEstablished)
                dev->second->wakeup();
            else
                dev->second->sleep();
            return;
        }

        if(!event.linkEstablished)
            return;

        hidpp::Device hidpp_device(receiver(), event);

        auto version = hidpp_device.version();

        if(std::get<0>(version) < 2) {
            logPrintf(INFO, "Unsupported HID++ 1.0 device on %s:%d connected.",
                    _path.c_str(), event.index);
            return;
        }

        auto device = Device::make(this, event.index, manager);
        std::lock_guard<std::mutex> manager_lock(manager->mutex());
        _devices.emplace(event.index, device);
        manager->addExternalDevice(device);

    } catch(hidpp10::Error &e) {
        logPrintf(ERROR,
                       "Caught HID++ 1.0 error while trying to initialize "
                       "%s:%d: %s", _path.c_str(), event.index, e.what());
    } catch(hidpp20::Error &e) {
        logPrintf(ERROR, "Caught HID++ 2.0 error while trying to initialize "
                          "%s:%d: %s", _path.c_str(), event.index, e.what());
    } catch(TimeoutError &e) {
        if(!event.fromTimeoutCheck)
            logPrintf(DEBUG, "%s:%d timed out, waiting for input from device to"
                             " initialize.", _path.c_str(), event.index);
        waitForDevice(event.index);
    }
}

void Receiver::removeDevice(hidpp::DeviceIndex index)
{
    std::unique_lock<std::mutex> lock(_devices_change);
    std::unique_lock<std::mutex> manager_lock;
    if(auto manager = _manager.lock())
        manager_lock = std::unique_lock<std::mutex>(manager->mutex());
    auto device = _devices.find(index);
    if(device != _devices.end()) {
        if(auto manager = _manager.lock())
            manager->removeExternalDevice(device->second);
        _devices.erase(device);
    }
}

const std::string& Receiver::path() const
{
    return _path;
}

std::shared_ptr<dj::Receiver> Receiver::rawReceiver()
{
    return receiver();
}

Receiver::ReceiverIPC::ReceiverIPC(Receiver *receiver) :
        ipcgull::interface("pizza.pixl.LogiOps.Receiver", {}, {}, {})
{
}
