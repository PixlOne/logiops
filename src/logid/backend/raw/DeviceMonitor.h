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

#ifndef LOGID_BACKEND_RAW_DEVICEMONITOR_H
#define LOGID_BACKEND_RAW_DEVICEMONITOR_H

#include <string>
#include <mutex>
#include <atomic>
#include <memory>

extern "C"
{
struct udev;
struct udev_monitor;
}

namespace logid::backend::raw {
    class IOMonitor;

    static constexpr int max_tries = 5;
    static constexpr int ready_backoff = 500;

    template<typename T>
    class _deviceMonitorWrapper : public T {
        friend class Device;

    public:
        template<typename... Args>
        explicit _deviceMonitorWrapper(Args... args) : T(std::forward<Args>(args)...) {}

        template<typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            return std::make_shared<_deviceMonitorWrapper>(std::forward<Args>(args)...);
        }
    };

    class DeviceMonitor {
    public:
        virtual ~DeviceMonitor();

        void enumerate();

        [[nodiscard]] std::shared_ptr<IOMonitor> ioMonitor() const;

        template<typename T, typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            auto device_monitor = _deviceMonitorWrapper<T>::make(std::forward<Args>(args)...);
            device_monitor->_self = device_monitor;
            device_monitor->ready();

            return device_monitor;
        }

    protected:
        DeviceMonitor();

        // This should be run once the derived class is ready
        void ready();

        virtual void addDevice(std::string device) = 0;

        virtual void removeDevice(std::string device) = 0;

        template<typename T>
        [[nodiscard]] std::weak_ptr<T> self() const {
            return std::dynamic_pointer_cast<T>(_self.lock());
        }

    private:
        void _addHandler(const std::string& device, int tries = 0);

        void _removeHandler(const std::string& device);

        std::shared_ptr<IOMonitor> _io_monitor;

        struct udev* _udev_context;
        struct udev_monitor* _udev_monitor;
        int _fd;
        bool _ready;

        std::weak_ptr<DeviceMonitor> _self;
    };
}

#endif //LOGID_BACKEND_RAW_DEVICEMONITOR_H