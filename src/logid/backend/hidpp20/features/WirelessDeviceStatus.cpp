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
#include <backend/hidpp20/features/WirelessDeviceStatus.h>
#include <cassert>
#include <stdexcept>

using namespace logid::backend::hidpp20;

WirelessDeviceStatus::WirelessDeviceStatus(Device* dev) : Feature(dev, ID) {
}

WirelessDeviceStatus::Status WirelessDeviceStatus::statusBroadcastEvent(
        const hidpp::Report& report) {
    if (report.function() != StatusBroadcast)
        throw std::runtime_error("statusBroadcastEvent: bad function");
    const auto param_size = static_cast<std::size_t>(report.paramEnd() - report.paramBegin());
    if (param_size < 3)
        throw std::runtime_error("statusBroadcastEvent: short report");
    Status status = {};
    auto params = report.paramBegin();
    status.reconnection = params[0];
    status.reconfNeeded = params[1];
    status.powerSwitch = params[2];
    return status;
}