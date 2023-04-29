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
#include <backend/hidpp20/features/HiresScroll.h>
#include <cassert>

using namespace logid::backend::hidpp20;

HiresScroll::HiresScroll(Device* device) : Feature(device, ID) {
}

HiresScroll::Capabilities HiresScroll::getCapabilities() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetCapabilities, params);

    Capabilities capabilities{};
    capabilities.multiplier = response[0];
    capabilities.flags = response[1];
    return capabilities;
}

uint8_t HiresScroll::getMode() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetMode, params);
    return response[0];
}

void HiresScroll::setMode(uint8_t mode) {
    std::vector<uint8_t> params(1);
    params[0] = mode;
    callFunction(SetMode, params);
}

[[maybe_unused]] bool HiresScroll::getRatchetState() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetRatchetState, params);
    return params[0];
}

HiresScroll::WheelStatus HiresScroll::wheelMovementEvent(const hidpp::Report& report) {
    assert(report.function() == WheelMovement);
    WheelStatus status{};
    status.hiRes = report.paramBegin()[0] & 1 << 4;
    status.periods = report.paramBegin()[0] & 0x0F;
    status.deltaV = (int16_t) (report.paramBegin()[1] << 8 | report.paramBegin()[2]);
    return status;
}

[[maybe_unused]]
HiresScroll::RatchetState HiresScroll::ratchetSwitchEvent(const hidpp::Report& report) {
    assert(report.function() == RatchetSwitch);
    // Possible bad cast
    return static_cast<RatchetState>(report.paramBegin()[0]);
}