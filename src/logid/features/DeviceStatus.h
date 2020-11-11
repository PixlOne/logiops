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
#ifndef LOGID_FEATURE_DEVICESTATUS_H
#define LOGID_FEATURE_DEVICESTATUS_H


#include "DeviceFeature.h"
#include "../Device.h"
#include "../backend/hidpp20/features/WirelessDeviceStatus.h"

namespace logid {
namespace features
{
    class DeviceStatus : public DeviceFeature
    {
    public:
        explicit DeviceStatus(Device* dev);
        virtual void configure();
        virtual void listen();
    private:
        std::shared_ptr<backend::hidpp20::WirelessDeviceStatus>
                _wireless_device_status;
    };
}}

#endif //LOGID_FEATURE_DEVICESTATUS_H
