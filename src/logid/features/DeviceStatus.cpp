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
#include <features/DeviceStatus.h>
#include <util/task.h>

using namespace logid::features;
using namespace logid::backend;

DeviceStatus::DeviceStatus(logid::Device* dev) : DeviceFeature(dev) {
    /* This feature is redundant on receivers since the receiver
     * handles wakeup/sleep events. If the device is connected on a
     * receiver, pretend this feature is unsupported.
     */
    if (dev->hidpp20().deviceIndex() >= hidpp::WirelessDevice1 &&
        dev->hidpp20().deviceIndex() <= hidpp::WirelessDevice6)
        throw UnsupportedFeature();

    try {
        _wireless_device_status = std::make_shared<hidpp20::WirelessDeviceStatus>(&dev->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }
}

void DeviceStatus::configure() {
    // Do nothing
}

void DeviceStatus::listen() {
    if (_ev_handler.empty()) {
        _ev_handler = _device->hidpp20().addEventHandler(
                {[index = _wireless_device_status->featureIndex()](
                        const hidpp::Report& report) -> bool {
                    return report.feature() == index &&
                           report.function() ==
                           hidpp20::WirelessDeviceStatus::StatusBroadcast;
                },
                 [self_weak = self<DeviceStatus>()](
                         const hidpp::Report& report) {
                     auto event = hidpp20::WirelessDeviceStatus::statusBroadcastEvent(report);
                     if (event.reconfNeeded)
                         run_task_after([self_weak]() {
                             if (auto self = self_weak.lock())
                                 self->_device->wakeup();
                         }, std::chrono::milliseconds(100));
                 }
                });
    }
}

void DeviceStatus::setProfile(config::Profile&) {
}
