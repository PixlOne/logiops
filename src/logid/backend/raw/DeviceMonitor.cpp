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
#include "../../util/thread.h"
#include "../../util/log.h"

#include <thread>
#include <system_error>

extern "C"
{
#include <unistd.h>
#include <libudev.h>
}

using namespace logid::backend::raw;

DeviceMonitor::DeviceMonitor()
{
    if(-1 == pipe(_pipe))
        throw std::system_error(errno, std::system_category(),
                "pipe creation failed");

    _udev_context = udev_new();
    if(!_udev_context)
        throw std::runtime_error("udev_new failed");
}

DeviceMonitor::~DeviceMonitor()
{
    this->stop();

    udev_unref(_udev_context);

    for(int i : _pipe)
        close(i);
}

void DeviceMonitor::run()
{
    int ret;
    std::lock_guard<std::mutex> lock(_running);

    struct udev_monitor* monitor = udev_monitor_new_from_netlink(_udev_context,
            "udev");
    if(!monitor)
        throw std::runtime_error("udev_monitor_new_from_netlink failed");

    ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw",
            nullptr);
    if (0 != ret)
        throw std::system_error (-ret, std::system_category(),
                "udev_monitor_filter_add_match_subsystem_devtype");

    ret = udev_monitor_enable_receiving(monitor);
    if(0 != ret)
        throw std::system_error(-ret, std::system_category(),
                "udev_moniotr_enable_receiving");

    this->enumerate();

    int fd = udev_monitor_get_fd(monitor);

    _run_monitor = true;
    while (_run_monitor) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(_pipe[0], &fds);
        FD_SET(fd, &fds);

        if (-1 == select (std::max (_pipe[0], fd)+1, &fds, nullptr,
                nullptr, nullptr)) {
            if (errno == EINTR)
                continue;
            throw std::system_error (errno, std::system_category(),
                    "udev_monitor select");
        }

        if (FD_ISSET(fd, &fds)) {
            struct udev_device *device = udev_monitor_receive_device(monitor);
            std::string action = udev_device_get_action(device);
            std::string devnode = udev_device_get_devnode(device);

            if (action == "add")
                thread::spawn([this, name=devnode]() {
                    this->addDevice(name);
                }, [name=devnode](std::exception& e){
                    logPrintf(WARN, "Error adding device %s: %s",
                            name.c_str(), e.what());
                });
            else if (action == "remove")
                thread::spawn([this, name=devnode]() {
                    this->removeDevice(name);
                }, [name=devnode](std::exception& e){
                    logPrintf(WARN, "Error removing device %s: %s",
                               name.c_str(), e.what());
                });

            udev_device_unref (device);
        }
        if (FD_ISSET(_pipe[0], &fds)) {
            char c;
            if (-1 == read(_pipe[0], &c, sizeof (char)))
                throw std::system_error (errno, std::system_category(),
                                         "read pipe");
            break;
        }
    }
}

void DeviceMonitor::stop()
{
    _run_monitor = false;
    std::lock_guard<std::mutex> lock(_running);
}

void DeviceMonitor::enumerate()
{
    int ret;
    struct udev_enumerate* udev_enum = udev_enumerate_new(_udev_context);
    ret = udev_enumerate_add_match_subsystem(udev_enum, "hidraw");
    if(0 != ret)
        throw std::system_error(-ret, std::system_category(),
                "udev_enumerate_add_match_subsystem");

    ret = udev_enumerate_scan_devices(udev_enum);
    if(0 != ret)
        throw std::system_error(-ret, std::system_category(),
                                "udev_enumerate_scan_devices");

    struct udev_list_entry* udev_enum_entry;
    udev_list_entry_foreach(udev_enum_entry,
            udev_enumerate_get_list_entry(udev_enum)) {
        const char* name = udev_list_entry_get_name(udev_enum_entry);

        struct udev_device* device = udev_device_new_from_syspath(_udev_context,
                name);
        if(!device)
            throw std::runtime_error("udev_device_new_from_syspath failed");

        std::string devnode = udev_device_get_devnode(device);
        udev_device_unref(device);

        thread::spawn([this, name=devnode]() {
            this->addDevice(name);
        }, [name=devnode](std::exception& e){
            logPrintf(ERROR, "Error adding device %s: %s",
                       name.c_str(), e.what());
        });
    }

    udev_enumerate_unref(udev_enum);
}