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

#include "../hidpp/Device.h"
#include "Error.h"

namespace logid::backend::hidpp20 {
    class Device : public hidpp::Device {
    public:
        Device(const std::string& path, hidpp::DeviceIndex index,
               std::shared_ptr<raw::DeviceMonitor> monitor, double timeout);

        Device(std::shared_ptr<raw::RawDevice> raw_device,
               hidpp::DeviceIndex index, double timeout);

        Device(const std::shared_ptr<dj::Receiver>& receiver,
               hidpp::DeviceConnectionEvent event, double timeout);

        Device(const std::shared_ptr<dj::Receiver>& receiver,
               hidpp::DeviceIndex index, double timeout);

        std::vector<uint8_t> callFunction(uint8_t feature_index,
                                          uint8_t function,
                                          std::vector<uint8_t>& params);

        void callFunctionNoResponse(uint8_t feature_index,
                                    uint8_t function,
                                    std::vector<uint8_t>& params);

        hidpp::Report sendReport(const hidpp::Report& report) final;

    protected:
        bool responseReport(const hidpp::Report& report) final;

    private:
        std::mutex _response_lock;
        std::mutex _response_wait_lock;
        std::condition_variable _response_cv;

        static constexpr int response_slots = 14;
        typedef std::variant<hidpp::Report, Error::ErrorCode> Response;
        std::map<uint8_t, std::optional<Response>> _responses;
    };
}

#endif //LOGID_BACKEND_HIDPP20_DEVICE_H