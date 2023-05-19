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

#ifndef LOGID_BACKEND_HIDPP_DEVICE_H
#define LOGID_BACKEND_HIDPP_DEVICE_H

#include <backend/raw/RawDevice.h>
#include <backend/hidpp/Report.h>
#include <backend/hidpp/defs.h>
#include <backend/EventHandlerList.h>
#include <optional>
#include <variant>
#include <string>
#include <memory>
#include <functional>
#include <map>

namespace logid::backend::hidpp10 {
    // Need to define here for a constructor
    class Receiver;
}

namespace logid::backend::hidpp {
    struct DeviceConnectionEvent;

    template<typename T>
    class _deviceWrapper : public T {
        friend class Device;

    public:
        template<typename... Args>
        explicit _deviceWrapper(Args... args) : T(std::forward<Args>(args)...) {}

        template<typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            return std::make_shared<_deviceWrapper>(std::forward<Args>(args)...);
        }
    };

    class Device {
        template<typename T>
        friend
        class _deviceWrapper;

    public:
        struct EventHandler {
            std::function<bool(Report&)> condition;
            std::function<void(Report&)> callback;
        };

        class InvalidDevice : std::exception {
        public:
            enum Reason {
                NoHIDPPReport,
                InvalidRawDevice,
                Asleep,
                VirtualNode
            };

            explicit InvalidDevice(Reason reason) : _reason(reason) {}

            [[nodiscard]] const char* what() const noexcept override;

            [[nodiscard]] virtual Reason code() const noexcept;

        private:
            Reason _reason;
        };

        [[nodiscard]] const std::string& devicePath() const;

        [[nodiscard]] DeviceIndex deviceIndex() const;

        [[nodiscard]] const std::tuple<uint8_t, uint8_t>& version() const;

        [[nodiscard]] const std::string& name() const;

        [[nodiscard]] uint16_t pid() const;

        EventHandlerLock<Device> addEventHandler(EventHandler handler);

        virtual Report sendReport(const Report& report);

        virtual void sendReportNoACK(const Report& report);

        void handleEvent(Report& report);

        [[nodiscard]] const std::shared_ptr<raw::RawDevice>& rawDevice() const;

        Device(const Device&) = delete;

        Device(Device&&) = delete;

        virtual ~Device() = default;

    protected:
        Device(const std::string& path, DeviceIndex index,
               const std::shared_ptr<raw::DeviceMonitor>& monitor, double timeout);

        Device(std::shared_ptr<raw::RawDevice> raw_device, DeviceIndex index,
               double timeout);

        Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceConnectionEvent event, double timeout);

        Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               DeviceIndex index, double timeout);

        // Returns whether the report is a response
        virtual bool responseReport(const Report& report);

        bool isStable20();

        bool isStable10();

        void _sendReport(Report report);

        void reportFixup(Report& report) const;

        const std::chrono::milliseconds io_timeout;
        uint8_t supported_reports{};

        std::mutex _response_mutex;
        std::condition_variable _response_cv;
    private:
        void _setupReportsAndInit();

        void _init();

        std::shared_ptr<raw::RawDevice> _raw_device;
        EventHandlerLock<raw::RawDevice> _raw_handler;
        std::shared_ptr<hidpp10::Receiver> _receiver;
        std::string _path;
        DeviceIndex _index;

        std::tuple<uint8_t, uint8_t> _version;
        uint16_t _pid{};
        std::string _name;

        std::mutex _send_mutex;

        typedef std::variant<Report, Report::Hidpp10Error, Report::Hidpp20Error> Response;

        std::optional<Response> _response;
        std::optional<uint8_t> _sent_sub_id{};
        std::optional<uint8_t> _sent_address{};

        std::shared_ptr<EventHandlerList<Device>> _event_handlers;

        std::weak_ptr<Device> _self;

    protected:
        template<typename T, typename... Args>
        static std::shared_ptr<T> makeDerived(Args... args) {
            auto device = _deviceWrapper<T>::make(std::forward<Args>(args)...);
            device->_self = device;
            device->_setupReportsAndInit();
            return device;
        }

    public:
        template<typename... Args>
        static std::shared_ptr<Device> make(Args... args) {
            return makeDerived<Device>(std::forward<Args>(args)...);
        }
    };

    typedef Device::EventHandler EventHandler;
}

#endif //LOGID_BACKEND_HIDPP_DEVICE_H