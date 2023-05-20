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

#ifndef LOGID_BACKEND_RAWDEVICE_H
#define LOGID_BACKEND_RAWDEVICE_H

#include <backend/raw/EventHandler.h>
#include <backend/EventHandlerList.h>
#include <string>
#include <vector>
#include <shared_mutex>
#include <atomic>
#include <future>
#include <set>
#include <list>

namespace logid::backend::raw {
    class DeviceMonitor;

    class IOMonitor;

    template <typename T>
    class RawDeviceWrapper : public T {
    public:
        template <typename... Args>
        RawDeviceWrapper(Args... args) : T(std::forward<Args>(args)...) { }
    };

    class RawDevice {
        template <typename>
        friend class RawDeviceWrapper;
    public:
        static constexpr int max_data_length = 32;
        typedef RawEventHandler EventHandler;

        enum BusType {
            USB,
            Bluetooth,
            OtherBus
        };

        struct dev_info {
            int16_t vid;
            int16_t pid;
            BusType bus_type;
        };

        template <typename... Args>
        static std::shared_ptr<RawDevice> make(Args... args) {
            auto raw_dev = std::make_shared<RawDeviceWrapper<RawDevice>>(
                    std::forward<Args>(args)...);
            raw_dev->_self = raw_dev;
            raw_dev->_ready();
            return raw_dev;
        }

        ~RawDevice() noexcept;

        [[nodiscard]] const std::string& rawPath() const;

        [[nodiscard]] const std::string& name() const;

        [[maybe_unused]]
        [[nodiscard]] int16_t vendorId() const;

        [[nodiscard]] int16_t productId() const;

        [[nodiscard]] BusType busType() const;

        [[nodiscard]] bool isSubDevice() const;

        static std::vector<uint8_t> getReportDescriptor(const std::string& path);

        static std::vector<uint8_t> getReportDescriptor(int fd);

        [[nodiscard]] const std::vector<uint8_t>& reportDescriptor() const;

        void sendReport(const std::vector<uint8_t>& report);

        [[nodiscard]] EventHandlerLock<RawDevice> addEventHandler(RawEventHandler handler);

    private:
        RawDevice(std::string path, const std::shared_ptr<DeviceMonitor>& monitor);

        void _ready();

        void _readReports();

        std::atomic_bool _valid;

        const std::string _path;
        const int _fd;
        const dev_info _dev_info;
        const std::string _name;
        const std::vector<uint8_t> _report_desc;

        std::shared_ptr<IOMonitor> _io_monitor;

        std::weak_ptr<RawDevice> _self;

        bool _sub_device = false;

        std::shared_ptr<EventHandlerList<RawDevice>> _event_handlers;

        void _handleEvent(const std::vector<uint8_t>& report);
    };
}

#endif //LOGID_BACKEND_RAWDEVICE_H
