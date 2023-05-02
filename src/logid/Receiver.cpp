/*
 * Copyright 2019-2023 PixlOne
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

#include <Receiver.h>
#include <DeviceManager.h>
#include <backend/Error.h>
#include <util/log.h>
#include <ipc_defs.h>

using namespace logid;
using namespace logid::backend;

ReceiverNickname::ReceiverNickname(
        const std::shared_ptr<DeviceManager>& manager) :
        _nickname(manager->newReceiverNickname()), _manager(manager) {
}

ReceiverNickname::operator std::string() const {
    return std::to_string(_nickname);
}

ReceiverNickname::~ReceiverNickname() {
    if (auto manager = _manager.lock()) {
        std::lock_guard<std::mutex> lock(manager->_nick_lock);
        manager->_receiver_nicknames.erase(_nickname);
    }
}

std::shared_ptr<Receiver> Receiver::make(
        const std::string& path,
        const std::shared_ptr<DeviceManager>& manager) {
    auto ret = ReceiverMonitor::make<Receiver>(path, manager);
    ret->_ipc_node->manage(ret);
    return ret;
}


Receiver::Receiver(const std::string& path,
                   const std::shared_ptr<DeviceManager>& manager) :
        hidpp10::ReceiverMonitor(path, manager,
                                 manager->config()->io_timeout.value_or(
                                         defaults::io_timeout)),
        _path(path), _manager(manager), _nickname(manager),
        _ipc_node(manager->receiversNode()->make_child(_nickname)),
        _ipc_interface(_ipc_node->make_interface<IPC>(this)) {
}

const Receiver::DeviceList& Receiver::devices() const {
    return _devices;
}

Receiver::~Receiver() noexcept {
    if (auto manager = _manager.lock()) {
        for (auto& d: _devices)
            manager->removeExternalDevice(d.second);
    }
}

void Receiver::addDevice(hidpp::DeviceConnectionEvent event) {
    std::unique_lock<std::mutex> lock(_devices_change);

    auto manager = _manager.lock();
    if (!manager) {
        logPrintf(ERROR, "Orphan Receiver, missing DeviceManager");
        logPrintf(ERROR, "Fatal error, file a bug report. Program will now exit.");
        std::terminate();
    }

    try {
        // Check if device is ignored before continuing
        if (manager->config()->ignore.value_or(std::set<uint16_t>()).contains(event.pid)) {
            logPrintf(DEBUG, "%s:%d: Device 0x%04x ignored.",
                      _path.c_str(), event.index, event.pid);
            return;
        }

        auto dev = _devices.find(event.index);
        if (dev != _devices.end()) {
            if (event.linkEstablished)
                dev->second->wakeup();
            else
                dev->second->sleep();
            return;
        }

        if (!event.linkEstablished)
            return;

        auto hidpp_device = hidpp::Device::make(
                receiver(), event, manager->config()->io_timeout.value_or(defaults::io_timeout));

        auto version = hidpp_device->version();

        if (std::get<0>(version) < 2) {
            logPrintf(INFO, "Unsupported HID++ 1.0 device on %s:%d connected.",
                      _path.c_str(), event.index);
            return;
        }

        hidpp_device.reset();

        auto device = Device::make(this, event.index, manager);
        std::lock_guard<std::mutex> manager_lock(manager->mutex());
        _devices.emplace(event.index, device);
        manager->addExternalDevice(device);

    } catch (hidpp10::Error& e) {
        logPrintf(ERROR, "Caught HID++ 1.0 error while trying to initialize %s:%d: %s",
                  _path.c_str(), event.index, e.what());
    } catch (hidpp20::Error& e) {
        logPrintf(ERROR, "Caught HID++ 2.0 error while trying to initialize "
                         "%s:%d: %s", _path.c_str(), event.index, e.what());
    } catch (TimeoutError& e) {
        if (!event.fromTimeoutCheck)
            logPrintf(DEBUG, "%s:%d timed out, waiting for input from device to"
                             " initialize.", _path.c_str(), event.index);
        waitForDevice(event.index);
    }
}

void Receiver::removeDevice(hidpp::DeviceIndex index) {
    std::unique_lock<std::mutex> lock(_devices_change);
    std::unique_lock<std::mutex> manager_lock;
    if (auto manager = _manager.lock())
        manager_lock = std::unique_lock<std::mutex>(manager->mutex());
    auto device = _devices.find(index);
    if (device != _devices.end()) {
        if (auto manager = _manager.lock())
            manager->removeExternalDevice(device->second);
        _devices.erase(device);
    }
}

void Receiver::pairReady(const hidpp10::DeviceDiscoveryEvent& event,
                         const std::string& passkey) {
    std::string type;
    switch (event.deviceType) {
        case hidpp::DeviceUnknown:
            type = "unknown";
            break;
        case hidpp::DeviceKeyboard:
            type = "keyboard";
            break;
        case hidpp::DeviceMouse:
            type = "mouse";
            break;
        case hidpp::DeviceNumpad:
            type = "numpad";
            break;
        case hidpp::DevicePresenter:
            type = "presenter";
            break;
        case hidpp::DeviceTouchpad:
            type = "touchpad";
            break;
        case hidpp::DeviceTrackball:
            type = "trackball";
            break;
    }
    _ipc_interface->emit_signal("PairReady", event.name, event.pid, type, passkey);
}

const std::string& Receiver::path() const {
    return _path;
}

std::shared_ptr<hidpp10::Receiver> Receiver::rawReceiver() {
    return receiver();
}

std::vector<std::tuple<int, uint16_t, std::string, uint32_t>> Receiver::pairedDevices() const {
    std::vector<std::tuple<int, uint16_t, std::string, uint32_t>> ret;
    for (int i = hidpp::WirelessDevice1; i <= hidpp::WirelessDevice6; ++i) {
        try {
            auto index(static_cast<hidpp::DeviceIndex>(i));
            auto pair_info = receiver()->getPairingInfo(index);
            auto extended_pair_info = receiver()->getExtendedPairingInfo(index);
            auto name = receiver()->getDeviceName(index);

            ret.emplace_back(i, pair_info.pid, name, extended_pair_info.serialNumber);
        } catch (hidpp10::Error& e) {
        }
    }

    return ret;
}

void Receiver::startPair(uint8_t timeout) {
    _startPair(timeout);
}

void Receiver::stopPair() {
    _stopPair();
}

void Receiver::unpair(int device) {
    receiver()->disconnect(static_cast<hidpp::DeviceIndex>(device));
}

Receiver::IPC::IPC(Receiver* receiver) :
        ipcgull::interface(
                SERVICE_ROOT_NAME ".Receiver",
                {
                        {"GetPaired",     {receiver, &Receiver::pairedDevices, {"devices"}}},
                        {"StartPair",     {receiver, &Receiver::startPair,     {"timeout"}}},
                        {"StopPair",      {receiver, &Receiver::stopPair}},
                        {"Unpair",        {receiver, &Receiver::unpair,        {"device"}}}
                },
                {
                        {"Bolt", ipcgull::property<bool>(ipcgull::property_readable,
                                                         receiver->receiver()->bolt())}
                }, {
                        {"PairReady",
                         ipcgull::signal::make_signal<std::string, uint16_t, std::string,
                            std::string>(
                                 {"name", "pid", "type", "passkey"})
                        }
                }) {
}
