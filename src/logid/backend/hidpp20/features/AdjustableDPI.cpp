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
#include <backend/hidpp20/features/AdjustableDPI.h>

using namespace logid::backend::hidpp20;

AdjustableDPI::AdjustableDPI(Device* dev) : Feature(dev, ID) {
}

uint8_t AdjustableDPI::getSensorCount() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetSensorCount, params);
    return response[0];
}

AdjustableDPI::SensorDPIList AdjustableDPI::getSensorDPIList(uint8_t sensor) {
    SensorDPIList dpi_list{};
    std::vector<uint8_t> params(1);
    params[0] = sensor;
    auto response = callFunction(GetSensorDPIList, params);

    dpi_list.dpiStep = false;
    for (std::size_t i = 1; i < response.size(); i += 2) {
        uint16_t dpi = response[i + 1];
        dpi |= (response[i] << 8);
        if (!dpi)
            break;
        if (dpi >= 0xe000) {
            dpi_list.isRange = true;
            dpi_list.dpiStep = dpi - 0xe000;
        } else {
            dpi_list.dpis.push_back(dpi);
        }
    }

    return dpi_list;
}

uint16_t AdjustableDPI::getDefaultSensorDPI(uint8_t sensor) {
    std::vector<uint8_t> params(1);
    params[0] = sensor;
    auto response = callFunction(GetSensorDPI, params);

    uint16_t default_dpi = response[4];
    default_dpi |= (response[3] << 8);

    return default_dpi;
}

uint16_t AdjustableDPI::getSensorDPI(uint8_t sensor) {
    std::vector<uint8_t> params(1);
    params[0] = sensor;
    auto response = callFunction(GetSensorDPI, params);

    uint16_t dpi = response[2];
    dpi |= (response[1] << 8);

    return dpi;
}

void AdjustableDPI::setSensorDPI(uint8_t sensor, uint16_t dpi) {
    std::vector<uint8_t> params(3);
    params[0] = sensor;
    params[1] = (dpi >> 8);
    params[2] = (dpi & 0xFF);
    callFunction(SetSensorDPI, params);
}