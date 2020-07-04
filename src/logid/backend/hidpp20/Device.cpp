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

#include <cassert>

#include "Device.h"
#include "../hidpp/defs.h"

using namespace logid::backend::hidpp20;

Device::Device(std::string path, hidpp::DeviceIndex index)
    : hidpp::Device(path, index)
{
    assert(std::get<0>(version()) >= 2);
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_device, hidpp::DeviceIndex index)
        : hidpp::Device(raw_device, index)
{
    assert(std::get<0>(version()) >= 2);
}

std::vector<uint8_t> Device::callFunction(uint8_t feature_index,
        uint8_t function, std::vector<uint8_t>& params)
{
    hidpp::Report::Type type;

    assert(params.size() <= hidpp::LongParamLength);
    if(params.size() <= hidpp::ShortParamLength)
        type = hidpp::Report::Type::Short;
    else if(params.size() <= hidpp::LongParamLength)
        type = hidpp::Report::Type::Long;
    else
        throw hidpp::Report::InvalidReportID();

    hidpp::Report request(type, deviceIndex(), feature_index, function,
            LOGID_HIDPP_SOFTWARE_ID);
    std::copy(params.begin(), params.end(), request.paramBegin());

    auto response = this->sendReport(request);
    return std::vector<uint8_t>(response.paramBegin(), response.paramEnd());
}