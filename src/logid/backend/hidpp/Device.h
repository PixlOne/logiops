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

        explicit Device(const std::string& path, DeviceIndex index);
        explicit Device(std::shared_ptr<raw::RawDevice> raw_device,
                DeviceIndex index);
        explicit Device(std::shared_ptr<dj::Receiver> receiver,
                hidpp::DeviceConnectionEvent event);
        ~Device();

        std::string devicePath() const;
        DeviceIndex deviceIndex() const;
        std::tuple<uint8_t, uint8_t> version() const;

        std::string name() const;
        uint16_t pid() const;

        void listen(); // Runs asynchronously
        void stopListening();

        void addEventHandler(const std::string& nickname,
                const std::shared_ptr<EventHandler>& handler);
        void removeEventHandler(const std::string& nickname);
        const std::map<std::string, std::shared_ptr<EventHandler>>&
            eventHandlers();

        Report sendReport(Report& report);

        void handleEvent(Report& report);
    private:
        void _init();

        std::shared_ptr<raw::RawDevice> _raw_device;
        std::shared_ptr<dj::Receiver> _receiver;
        std::string _path;
        DeviceIndex _index;
        uint8_t _supported_reports;

        std::tuple<uint8_t, uint8_t> _version;
        uint16_t _pid;
        std::string _name;

        std::atomic<bool> _listening;

        std::map<std::string, std::shared_ptr<EventHandler>> _event_handlers;
    };
} } }

#endif //LOGID_BACKEND_HIDPP_DEVICE_H