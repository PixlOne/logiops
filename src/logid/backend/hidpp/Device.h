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

#ifndef LOGID_BACKEND_HIDPP_DEVICE_H
#define LOGID_BACKEND_HIDPP_DEVICE_H

#include <optional>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include "../raw/RawDevice.h"
#include "Report.h"
#include "defs.h"

namespace logid {
namespace backend {
namespace dj
{
    // Need to define here for a constructor
    class Receiver;
}
namespace hidpp
{
    struct DeviceConnectionEvent;
    struct EventHandler
    {
        std::function<bool(Report&)> condition;
        std::function<void(Report&)> callback;
    };
    class Device
    {
    public:
        class InvalidDevice : std::exception
        {
        public:
            enum Reason
            {
                NoHIDPPReport,
                InvalidRawDevice,
                Asleep
            };
            InvalidDevice(Reason reason) : _reason (reason) {}
            virtual const char* what() const noexcept;
            virtual Reason code() const noexcept;
        private:
            Reason _reason;
        };

        Device(const std::string& path, DeviceIndex index,
               std::shared_ptr<raw::DeviceMonitor> monitor, double timeout);
        Device(std::shared_ptr<raw::RawDevice> raw_device, DeviceIndex index,
               double timeout);
        Device(std::shared_ptr<dj::Receiver> receiver,
               hidpp::DeviceConnectionEvent event, double timeout);
        Device(std::shared_ptr<dj::Receiver> receiver,
               DeviceIndex index, double timeout);
        virtual ~Device();

        std::string devicePath() const;
        DeviceIndex deviceIndex() const;
        std::tuple<uint8_t, uint8_t> version() const;

        std::string name() const;
        uint16_t pid() const;

        void addEventHandler(const std::string& nickname,
                const std::shared_ptr<EventHandler>& handler);
        void removeEventHandler(const std::string& nickname);
        const std::map<std::string, std::shared_ptr<EventHandler>>&
            eventHandlers();

        virtual Report sendReport(const Report& report);
        void sendReportNoResponse(Report report);

        void handleEvent(Report& report);
    protected:
        // Returns whether the report is a response
        virtual bool responseReport(const Report& report);

        void reportFixup(Report& report);

        const std::chrono::milliseconds io_timeout;
        uint8_t supported_reports;
    private:
        void _init();

        std::shared_ptr<raw::RawDevice> _raw_device;
        raw::RawDevice::EvHandlerId _raw_handler;
        std::shared_ptr<dj::Receiver> _receiver;
        std::string _path;
        DeviceIndex _index;

        std::tuple<uint8_t, uint8_t> _version;
        uint16_t _pid;
        std::string _name;

        std::mutex _send_lock;
        std::mutex _resp_wait_lock;
        std::condition_variable _resp_cv;
        std::mutex _slot_lock;
        std::optional<Report> _report_slot;

        std::map<std::string, std::shared_ptr<EventHandler>> _event_handlers;
    };
} } }

#endif //LOGID_BACKEND_HIDPP_DEVICE_H