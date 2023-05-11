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

#ifndef LOGID_BACKEND_HIDPP20_DEVICE_H
#define LOGID_BACKEND_HIDPP20_DEVICE_H

#include <cstdint>
#include <optional>
#include <variant>
#include <backend/hidpp20/Error.h>
#include <backend/hidpp/Device.h>

namespace logid::backend::hidpp20 {
    class Device : public hidpp::Device {
    public:
        std::vector<uint8_t> callFunction(uint8_t feature_index,
                                          uint8_t function,
                                          std::vector<uint8_t>& params);

        void callFunctionNoResponse(uint8_t feature_index,
                                    uint8_t function,
                                    std::vector<uint8_t>& params);

        hidpp::Report sendReport(const hidpp::Report& report) final;

        void sendReportNoACK(const hidpp::Report& report) final;

    protected:
        Device(const std::string& path, hidpp::DeviceIndex index,
               const std::shared_ptr<raw::DeviceMonitor>& monitor, double timeout);

        Device(std::shared_ptr<raw::RawDevice> raw_device,
               hidpp::DeviceIndex index, double timeout);

        Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceConnectionEvent event, double timeout);

        Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceIndex index, double timeout);

        bool responseReport(const hidpp::Report& report) final;

    private:
        typedef std::variant<hidpp::Report, hidpp::Report::Hidpp20Error> Response;
        struct ResponseSlot {
            std::optional<Response> response;
            std::optional<uint8_t> feature;
            void reset();
        };

        /* Multiplex responses on lower nibble of SubID, ignore upper nibble for space */
        std::array<ResponseSlot, 16> _responses;

    public:
        template <typename... Args>
        static std::shared_ptr<Device> make(Args... args) {
            auto device = makeDerived<Device>(std::forward<Args>(args)...);

            if (std::get<0>(device->version()) < 2)
                throw std::invalid_argument("not a hid++ 2.0 device");

            return device;
        }
    };
}

#endif //LOGID_BACKEND_HIDPP20_DEVICE_H