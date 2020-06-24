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

#ifndef LOGID_IPC_UTIL_H
#define LOGID_IPC_UTIL_H

#include "../Device.h"
#include <hidpp/defs.h>
#include <string>

namespace logid::IPC
{
    struct deviceInfo {
        std::string path;
        HIDPP::DeviceIndex index;
    };
    deviceInfo parseDevName(std::string name);
    std::string toDevName(deviceInfo devInfo);
    std::string toDevName(std::string path, HIDPP::DeviceIndex index);
    std::string toDBusPath(logid::Device* device);
    std::string toDBusPath(std::string path, HIDPP::DeviceIndex index);
}

#endif //LOGID_IPC_UTIL_H