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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_WIRELESSDEVICESTATUS_H
#define LOGID_BACKEND_HIDPP20_FEATURE_WIRELESSDEVICESTATUS_H

#include "../Feature.h"
#include "../feature_defs.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class WirelessDeviceStatus : public Feature
    {
    public:
        static constexpr uint16_t ID = FeatureID::WIRELESS_DEVICE_STATUS;
        virtual uint16_t getID() { return ID; }

        WirelessDeviceStatus(Device* dev);

        enum Event : uint8_t
        {
            StatusBroadcast = 0
        };

        struct Status
        {
            bool reconnection;
            bool reconfNeeded;
            bool powerSwitch;
        };

        static Status statusBroadcastEvent(const hidpp::Report &report);
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_WIRELESSDEVICESTATUS_H
