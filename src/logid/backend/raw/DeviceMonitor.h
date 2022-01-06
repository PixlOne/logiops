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

extern "C"
{
#include <libudev.h>
}

namespace logid {
namespace backend {
namespace raw
{
    class DeviceMonitor
    {
    public:
        void enumerate();
        void run();
        void stop();
    protected:
        DeviceMonitor();
        virtual ~DeviceMonitor();
        virtual void addDevice(std::string device) = 0;
        virtual void removeDevice(std::string device) = 0;
    private:
        struct udev* _udev_context;
        int _pipe[2];
        std::atomic<bool> _run_monitor;
        std::mutex _running;
    };
}}}

#endif //LOGID_BACKEND_RAW_DEVICEMONITOR_H