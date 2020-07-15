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

#ifndef LOGID_BACKEND_HIDPP10_DEVICE_H
#define LOGID_BACKEND_HIDPP10_DEVICE_H

#include "../hidpp/Device.h"

namespace logid {
namespace backend {
namespace hidpp10
{
    class Device : public hidpp::Device
    {
    public:
        Device(const std::string& path, hidpp::DeviceIndex index);
        Device(std::shared_ptr<raw::RawDevice> raw_dev,
                hidpp::DeviceIndex index);

        std::vector<uint8_t> getRegister(uint8_t address,
                const std::vector<uint8_t>& params, hidpp::Report::Type type);

        std::vector<uint8_t> setRegister(uint8_t address,
                const std::vector<uint8_t>& params, hidpp::Report::Type type);
    private:
        std::vector<uint8_t> accessRegister(uint8_t sub_id,
                uint8_t address, const std::vector<uint8_t>& params);
    };
}}}

#endif //LOGID_BACKEND_HIDPP10_DEVICE_H