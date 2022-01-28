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

#include <cmath>
#include "DeviceName.h"

using namespace logid::backend;
using namespace logid::backend::hidpp20;

DeviceName::DeviceName(Device* dev) : Feature(dev, ID)
{
}

uint8_t DeviceName::getNameLength()
{
    std::vector<uint8_t> params(0);

    auto response = this->callFunction(Function::GetLength, params);
    return response[0];
}

std::string _getName(uint8_t length,
        const std::function<std::vector<uint8_t>(std::vector<uint8_t>)>& fcall)
{
    uint8_t function_calls = length/hidpp::LongParamLength;
    if(length % hidpp::LongParamLength)
        function_calls++;
    std::vector<uint8_t> params(1);
    std::string name;

    for(uint8_t i = 0; i < function_calls; i++) {
        params[0] = i*hidpp::LongParamLength;
        auto name_section = fcall(params);
        for(std::size_t j = 0; j < hidpp::LongParamLength; j++) {
            if(params[0] + j >= length)
                return name;
            name += name_section[j];
        }
    }

    return name;
}

std::string DeviceName::getName()
{
    return _getName(getNameLength(), [this]
    (std::vector<uint8_t> params)->std::vector<uint8_t> {
        return this->callFunction(Function::GetDeviceName, params);
    });
}

EssentialDeviceName::EssentialDeviceName(hidpp::Device* dev) :
    EssentialFeature(dev, ID)
{
}

uint8_t EssentialDeviceName::getNameLength()
{
    std::vector<uint8_t> params(0);

    auto response = this->callFunction(DeviceName::Function::GetLength, params);

    if(response[0] == 1)
        return 1;

    return response[0];
}

std::string EssentialDeviceName::getName()
{
    return _getName(getNameLength(), [this]
    (std::vector<uint8_t> params)->std::vector<uint8_t> {
        return this->callFunction(DeviceName::Function::GetDeviceName, params);
    });
}