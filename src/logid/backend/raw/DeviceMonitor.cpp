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

#include "DeviceMonitor.h"
#include "../../util/task.h"
#include "../../util/log.h"
#include "RawDevice.h"
#include "../hidpp/Device.h"

#include <system_error>

extern "C"
{
#include <libudev.h>
}

using namespace logid;
using namespace logid::backend::raw;

DeviceMonitor::DeviceMonitor() : _io_monitor(std::make_shared<IOMonitor>()),
                                 _ready(false) {
    int ret;
    _udev_context = udev_new();
    if (!_udev_context)
        throw std::runtime_error("udev_new failed");

    _udev_monitor = udev_monitor_new_from_netlink(_udev_context,
                                                  "udev");
    if (!_udev_monitor) {
        if (_udev_context)
            udev_unref(_udev_context);
        throw std::runtime_error("udev_monitor_new_from_netlink failed");
    }

    ret = udev_monitor_filter_add_match_subsystem_devtype(
            _udev_monitor, "hidraw", nullptr);
    if (0 != ret) {
        if (_udev_monitor)
            udev_monitor_unref(_udev_monitor);
        if (_udev_context)
            udev_unref(_udev_context);
        throw std::system_error(
                -ret, std::system_category(),
                "udev_monitor_filter_add_match_subsystem_devtype");
    }

    ret = udev_monitor_enable_receiving(_udev_monitor);
    if (0 != ret) {
        if (_udev_monitor)
            udev_monitor_unref(_udev_monitor);
        if (_udev_context)
            udev_unref(_udev_context);
        throw std::system_error(-ret, std::system_category(),
                                "udev_monitor_enable_receiving");
    }

    _fd = udev_monitor_get_fd(_udev_monitor);
}

DeviceMonitor::~DeviceMonitor() {
    if (_ready)
        _io_monitor->remove(_fd);

    if (_udev_monitor)
        udev_monitor_unref(_udev_monitor);
    if (_udev_context)
        udev_unref(_udev_context);
}

void DeviceMonitor::ready() {
    if (_ready)
        return;
    _ready = true;

    _io_monitor->add(_fd, {
            [this]() {
                struct udev_device* device = udev_monitor_receive_device(
                        _udev_monitor);
                std::string action = udev_device_get_action(device);
                std::string dev_node = udev_device_get_devnode(device);

                if (action == "add")
                    spawn_task([this, dev_node]() { _addHandler(dev_node); });
                else if (action == "remove")
                    spawn_task([this, dev_node]() { _removeHandler(dev_node); });

                udev_device_unref(device);
            },
            []() {
                throw std::runtime_error("udev hangup");
            },
            []() {
                throw std::runtime_error("udev error");
            }
    });
}

void DeviceMonitor::enumerate() {
    int ret;
    struct udev_enumerate* udev_enum = udev_enumerate_new(_udev_context);
    ret = udev_enumerate_add_match_subsystem(udev_enum, "hidraw");
    if (0 != ret)
        throw std::system_error(-ret, std::system_category(),
                                "udev_enumerate_add_match_subsystem");

    ret = udev_enumerate_scan_devices(udev_enum);
    if (0 != ret)
        throw std::system_error(-ret, std::system_category(),
                                "udev_enumerate_scan_devices");

    struct udev_list_entry* udev_enum_entry;
    udev_list_entry_foreach(udev_enum_entry,
                            udev_enumerate_get_list_entry(udev_enum)) {
        const char* name = udev_list_entry_get_name(udev_enum_entry);

        struct udev_device* device = udev_device_new_from_syspath(_udev_context,
                                                                  name);
        if (!device)
            throw std::runtime_error("udev_device_new_from_syspath failed");

        std::string dev_node = udev_device_get_devnode(device);
        udev_device_unref(device);

        _addHandler(dev_node);
    }

    udev_enumerate_unref(udev_enum);
}

void DeviceMonitor::_addHandler(const std::string& device) {
    try {
        auto supported_reports = backend::hidpp::getSupportedReports(
                RawDevice::getReportDescriptor(device));
        if (supported_reports)
            this->addDevice(device);
        else
            logPrintf(DEBUG, "Unsupported device %s ignored",
                      device.c_str());
    } catch (std::exception& e) {
        logPrintf(WARN, "Error adding device %s: %s",
                  device.c_str(), e.what());
    }
}

void DeviceMonitor::_removeHandler(const std::string& device) {
    try {
        this->removeDevice(device);
    } catch (std::exception& e) {
        logPrintf(WARN, "Error removing device %s: %s",
                  device.c_str(), e.what());
    }
}

std::shared_ptr<IOMonitor> DeviceMonitor::ioMonitor() const {
    return _io_monitor;
}
