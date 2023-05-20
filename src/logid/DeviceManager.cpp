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

#include <DeviceManager.h>
#include <backend/Error.h>
#include <util/log.h>
#include <thread>
#include <sstream>
#include <utility>
#include <ipc_defs.h>

using namespace logid;
using namespace logid::backend;

DeviceManager::DeviceManager(std::shared_ptr<Configuration> config,
                             std::shared_ptr<InputDevice> virtual_input,
                             std::shared_ptr<ipcgull::server> server) :
        backend::raw::DeviceMonitor(),
        _server(std::move(server)), _config(std::move(config)),
        _virtual_input(std::move(virtual_input)),
        _root_node(ipcgull::node::make_root("")),
        _device_node(ipcgull::node::make_root("devices")),
        _receiver_node(ipcgull::node::make_root("receivers")) {
    _ipc_devices = _root_node->make_interface<DevicesIPC>(this);
    _ipc_receivers = _root_node->make_interface<ReceiversIPC>(this);
    _ipc_config = _root_node->make_interface<Configuration::IPC>(_config.get());
    _device_node->add_server(_server);
    _receiver_node->add_server(_server);
    _root_node->add_server(_server);
}

std::shared_ptr<Configuration> DeviceManager::config() const {
    return _config;
}

std::shared_ptr<InputDevice> DeviceManager::virtualInput() const {
    return _virtual_input;
}

std::shared_ptr<const ipcgull::node> DeviceManager::devicesNode() const {
    return _device_node;
}

std::shared_ptr<const ipcgull::node> DeviceManager::receiversNode() const {
    return _receiver_node;
}

void DeviceManager::addDevice(std::string path) {
    bool defaultExists = true;
    bool isReceiver = false;

    // Check if device is ignored before continuing
    {
        auto raw_dev = raw::RawDevice::make(path, self<DeviceManager>().lock());
        if (config()->ignore.has_value() &&
            config()->ignore.value().contains(raw_dev->productId())) {
            logPrintf(DEBUG, "%s: Device 0x%04x ignored.",
                      path.c_str(), raw_dev->productId());
            return;
        }
    }

    try {
        auto device = hidpp::Device::make(
                path, hidpp::DefaultDevice, self<DeviceManager>().lock(),
                config()->io_timeout.value_or(defaults::io_timeout));
        isReceiver = device->version() == std::make_tuple(1, 0);
    } catch (hidpp20::Error& e) {
        if (e.code() != hidpp20::Error::UnknownDevice)
            throw DeviceNotReady();
        defaultExists = false;
    } catch (hidpp10::Error& e) {
        if (e.code() != hidpp10::Error::UnknownDevice)
            throw DeviceNotReady();
        defaultExists = false;
    } catch (hidpp::Device::InvalidDevice& e) {
        if (e.code() == hidpp::Device::InvalidDevice::VirtualNode) {
            logPrintf(DEBUG, "Ignoring virtual node on %s", path.c_str());
        } else if (e.code() == hidpp::Device::InvalidDevice::Asleep) {
            /* May be a valid device, wait */
            throw DeviceNotReady();
        }

        return;
    } catch (std::system_error& e) {
        logPrintf(WARN, "I/O error on %s: %s, skipping device.", path.c_str(), e.what());
        return;
    } catch (TimeoutError& e) {
        /* Ready and valid non-default devices should throw an UnknownDevice error */
        throw DeviceNotReady();
    }

    if (isReceiver) {
        logPrintf(INFO, "Detected receiver at %s", path.c_str());
        auto receiver = Receiver::make(path, self<DeviceManager>().lock());
        std::lock_guard<std::mutex> lock(_map_lock);
        _receivers.emplace(path, receiver);
        _ipc_receivers->receiverAdded(receiver);
    } else {
        /* TODO: Can non-receivers only contain 1 device?
        * If the device exists, it is guaranteed to be an HID++ 2.0 device */
        if (defaultExists) {
            auto device = Device::make(path, hidpp::DefaultDevice, self<DeviceManager>().lock());
            std::lock_guard<std::mutex> lock(_map_lock);
            _devices.emplace(path, device);
            _ipc_devices->deviceAdded(device);
        } else {
            try {
                auto device = Device::make(path, hidpp::CordedDevice, self<DeviceManager>().lock());
                std::lock_guard<std::mutex> lock(_map_lock);
                _devices.emplace(path, device);
                _ipc_devices->deviceAdded(device);
            } catch (hidpp10::Error& e) {
                if (e.code() != hidpp10::Error::UnknownDevice)
                    throw DeviceNotReady();
            } catch (hidpp20::Error& e) {
                if (e.code() != hidpp20::Error::UnknownDevice)
                    throw DeviceNotReady();
            } catch (hidpp::Device::InvalidDevice& e) {
                if (e.code() == hidpp::Device::InvalidDevice::Asleep)
                    throw DeviceNotReady();
            } catch (std::system_error& e) {
                // This error should have been thrown previously
                logPrintf(WARN, "I/O error on %s: %s", path.c_str(), e.what());
            } catch (TimeoutError& e) {
                throw DeviceNotReady();
            }
        }
    }
}

void DeviceManager::addExternalDevice(const std::shared_ptr<Device>& d) {
    _ipc_devices->deviceAdded(d);
}

void DeviceManager::removeExternalDevice(const std::shared_ptr<Device>& d) {
    _ipc_devices->deviceRemoved(d);
}

std::mutex& DeviceManager::mutex() const {
    return _map_lock;
}

void DeviceManager::removeDevice(std::string path) {
    std::lock_guard<std::mutex> lock(_map_lock);
    auto receiver = _receivers.find(path);

    if (receiver != _receivers.end()) {
        _ipc_receivers->receiverRemoved(receiver->second);
        _receivers.erase(receiver);
        logPrintf(INFO, "Receiver on %s disconnected", path.c_str());
    } else {
        auto device = _devices.find(path);
        if (device != _devices.end()) {
            _ipc_devices->deviceRemoved(device->second);
            _devices.erase(device);
            logPrintf(INFO, "Device on %s disconnected", path.c_str());
        }
    }
}

DeviceManager::DevicesIPC::DevicesIPC(DeviceManager* manager) :
        ipcgull::interface(
                SERVICE_ROOT_NAME ".Devices",
                {
                        {"Enumerate", {manager, &DeviceManager::listDevices, {"devices"}}}
                },
                {},
                {
                        {"DeviceAdded",
                                ipcgull::make_signal<std::shared_ptr<Device>>(
                                        {"device"})},
                        {"DeviceRemoved",
                                ipcgull::make_signal<std::shared_ptr<Device>>(
                                        {"device"})}
                }) {
}

std::vector<std::shared_ptr<Device>> DeviceManager::listDevices() const {
    std::lock_guard<std::mutex> lock(_map_lock);
    std::vector<std::shared_ptr<Device>> devices;
    for (auto& x: _devices)
        devices.emplace_back(x.second);
    for (auto& x: _receivers) {
        for (auto& d: x.second->devices())
            devices.emplace_back(d.second);
    }

    return devices;
}

std::vector<std::shared_ptr<Receiver>> DeviceManager::listReceivers() const {
    std::lock_guard<std::mutex> lock(_map_lock);
    std::vector<std::shared_ptr<Receiver>> receivers;
    for (auto& x: _receivers)
        receivers.emplace_back(x.second);
    return receivers;
}

void DeviceManager::DevicesIPC::deviceAdded(
        const std::shared_ptr<Device>& d) {
    emit_signal("DeviceAdded", d);
}

void DeviceManager::DevicesIPC::deviceRemoved(
        const std::shared_ptr<Device>& d) {
    emit_signal("DeviceRemoved", d);
}

DeviceManager::ReceiversIPC::ReceiversIPC(DeviceManager* manager) :
        ipcgull::interface(
                SERVICE_ROOT_NAME ".Receivers",
                {
                        {"Enumerate", {manager, &DeviceManager::listReceivers,
                                       {"receivers"}}}
                },
                {},
                {
                        {"ReceiverAdded",
                                ipcgull::make_signal<std::shared_ptr<Receiver>>(
                                        {"receiver"})},
                        {"ReceiverRemoved",
                                ipcgull::make_signal<std::shared_ptr<Receiver>>(
                                        {"receiver"})}
                }) {
}

void DeviceManager::ReceiversIPC::receiverAdded(
        const std::shared_ptr<Receiver>& r) {
    emit_signal("ReceiverAdded", r);
}

void DeviceManager::ReceiversIPC::receiverRemoved(
        const std::shared_ptr<Receiver>& r) {
    emit_signal("ReceiverRemoved", r);
}

int DeviceManager::newDeviceNickname() {
    std::lock_guard<std::mutex> lock(_nick_lock);

    auto begin = _device_nicknames.begin();
    if (begin != _device_nicknames.end()) {
        if (*begin != 0) {
            _device_nicknames.insert(0);
            return 0;
        }
    }

    const auto i = std::adjacent_find(_device_nicknames.begin(),
                                      _device_nicknames.end(),
                                      [](int l, int r) { return l + 1 < r; });


    if (i == _device_nicknames.end()) {
        auto end = _device_nicknames.rbegin();
        if (end != _device_nicknames.rend()) {
            auto ret = *end + 1;
            assert(ret > 0);
            _device_nicknames.insert(ret);
            return ret;
        } else {
            _device_nicknames.insert(0);
            return 0;
        }
    }

    auto ret = *i + 1;
    assert(ret > 0);
    _device_nicknames.insert(ret);
    return ret;
}

int DeviceManager::newReceiverNickname() {
    std::lock_guard<std::mutex> lock(_nick_lock);

    auto begin = _receiver_nicknames.begin();
    if (begin != _receiver_nicknames.end()) {
        if (*begin != 0) {
            _receiver_nicknames.insert(0);
            return 0;
        }
    }

    const auto i = std::adjacent_find(_receiver_nicknames.begin(),
                                      _receiver_nicknames.end(),
                                      [](int l, int r) { return l + 1 < r; });

    if (i == _receiver_nicknames.end()) {
        auto end = _receiver_nicknames.rbegin();
        if (end != _receiver_nicknames.rend()) {
            auto ret = *end + 1;
            assert(ret > 0);
            _receiver_nicknames.insert(ret);
            return ret;
        } else {
            _receiver_nicknames.insert(0);
            return 0;
        }
    }

    auto ret = *i + 1;
    assert(ret > 0);
    _receiver_nicknames.insert(ret);
    return ret;
}
