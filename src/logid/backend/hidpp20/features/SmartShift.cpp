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
#include <backend/hidpp20/features/SmartShift.h>

using namespace logid::backend::hidpp20;

SmartShift::SmartShift(Device* dev) : SmartShift(dev, ID) {
}

SmartShift::SmartShift(Device* dev, uint16_t feature_id) :
        Feature(dev, feature_id) {
}

SmartShiftV2::SmartShiftV2(Device* dev) : SmartShift(dev, ID) {
}

template<typename T>
std::shared_ptr<T> make_smartshift(Device* dev) {
    try {
        return std::make_shared<T>(dev);
    } catch (UnsupportedFeature& e) {
        return {};
    }
}

std::shared_ptr<SmartShift> SmartShift::autoVersion(Device* dev) {
    if (auto v2 = make_smartshift<SmartShiftV2>(dev))
        return v2;

    return std::make_shared<SmartShift>(dev);
}

SmartShift::Status SmartShift::getStatus() {
    std::vector<uint8_t> params(0);

    auto response = callFunction(GetStatus, params);

    return {
            .active = static_cast<bool>(response[0] - 1),
            .autoDisengage = response[1],
            .torque = 0,
            .setActive = false,
            .setAutoDisengage = false,
            .setTorque = false
    };
}

SmartShift::Defaults SmartShift::getDefaults() {
    std::vector<uint8_t> params(0);

    auto response = callFunction(GetStatus, params);

    return {
            .autoDisengage = response[2],
            .torque = 0,
            .maxForce = 0,
    };
}

void SmartShift::setStatus(Status status) {
    std::vector<uint8_t> params(3);
    if (status.setActive)
        params[0] = status.active + 1;
    if (status.setAutoDisengage)
        params[1] = status.autoDisengage;
    callFunction(SetStatus, params);
}

SmartShift::Defaults SmartShiftV2::getDefaults() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetCapabilities, params);

    return {
            .autoDisengage = response[1],
            .torque = response[2],
            .maxForce = response[3],
    };
}

SmartShift::Status SmartShiftV2::getStatus() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetStatus, params);

    return {
            .active = static_cast<bool>(response[0] - 1),
            .autoDisengage = response[1],
            .torque = response[2],
            .setActive = false, .setAutoDisengage = false, .setTorque = false,
    };
}

void SmartShiftV2::setStatus(Status status) {
    std::vector<uint8_t> params(3);
    if (status.setActive)
        params[0] = status.active + 1;
    if (status.setAutoDisengage)
        params[1] = status.autoDisengage;
    if (status.setTorque)
        params[2] = status.torque;

    callFunction(SetStatus, params);
}

bool SmartShiftV2::supportsTorque() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetCapabilities, params);

    return static_cast<bool>(response[0] & 1);
}

