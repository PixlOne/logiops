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

#include <backend/hidpp20/features/DeviceName.h>

using namespace logid::backend;
using namespace logid::backend::hidpp20;

namespace {
    std::string _getName(uint8_t length,
                         const std::function<std::vector<uint8_t>(std::vector<uint8_t>)>& fcall) {
        uint8_t function_calls = length / hidpp::LongParamLength;
        if (length % hidpp::LongParamLength)
            function_calls++;
        std::vector<uint8_t> params(1);
        std::string name;

        for (uint8_t i = 0; i < function_calls; i++) {
            params[0] = i * hidpp::LongParamLength;
            auto name_section = fcall(params);
            for (std::size_t j = 0; j < hidpp::LongParamLength; j++) {
                if (params[0] + j >= length)
                    return name;
                name += (char) name_section[j];
            }
        }

        return name;
    }
}

DeviceName::DeviceName(hidpp::Device* dev) :
        EssentialFeature(dev, ID) {
}

uint8_t DeviceName::getNameLength() {
    std::vector<uint8_t> params(0);

    auto response = this->callFunction(DeviceName::Function::GetLength, params);

    return response[0];
}

std::string DeviceName::getName() {
    return _getName(getNameLength(), [this]
            (std::vector<uint8_t> params) -> std::vector<uint8_t> {
        return this->callFunction(DeviceName::Function::GetDeviceName, params);
    });
}