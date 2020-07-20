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

#ifndef LOGID_BACKEND_HIDPP20_DEVICE_H
#define LOGID_BACKEND_HIDPP20_DEVICE_H

#include "../hidpp/Device.h"
#include <cstdint>

namespace logid {
namespace backend {
namespace hidpp20 {
    class Device : public hidpp::Device
    {
    public:
        Device(std::string path, hidpp::DeviceIndex index);
        Device(std::shared_ptr<raw::RawDevice> raw_device, hidpp::DeviceIndex index);
        Device(std::shared_ptr<dj::Receiver> receiver, hidpp::DeviceIndex
            index);

        std::vector<uint8_t> callFunction(uint8_t feature_index,
                uint8_t function,
                std::vector<uint8_t>& params);

        void callFunctionNoResponse(uint8_t feature_index,
                uint8_t function,
                std::vector<uint8_t>& params);
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_DEVICE_H