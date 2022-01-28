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

#ifndef LOGID_BACKEND_RAW_DEVICEMONITOR_H
#define LOGID_BACKEND_RAW_DEVICEMONITOR_H

#include <string>
#include <mutex>
#include <atomic>
#include <memory>
#include "IOMonitor.h"

extern "C"
{
    struct udev;
    struct udev_monitor;
}

namespace logid::backend::raw
{
    class DeviceMonitor
    {
    public:
        virtual ~DeviceMonitor();

        void enumerate();
        [[nodiscard]] std::shared_ptr<IOMonitor> ioMonitor() const;

    protected:
        DeviceMonitor();
        // This should be run once the derived class is ready
        void ready();
        virtual void addDevice(std::string device) = 0;
        virtual void removeDevice(std::string device) = 0;
    private:
        void _addHandler(const std::string& device);
        void _removeHandler(const std::string& device);

        std::shared_ptr<IOMonitor> _io_monitor;

        struct udev* _udev_context;
        struct udev_monitor* _udev_monitor;
        int _fd;
        bool _ready;
    };
}

#endif //LOGID_BACKEND_RAW_DEVICEMONITOR_H