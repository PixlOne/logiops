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

#ifndef LOGID_HIDPP_DEFS_H
#define LOGID_HIDPP_DEFS_H

#define LOGID_HIDPP_SOFTWARE_ID 0

#include <cstdint>

namespace logid {
namespace backend {
namespace hidpp
{
    namespace ReportType
    {
        enum ReportType : uint8_t
        {
            Short = 0x10,
            Long = 0x11
        };
    }

    enum DeviceIndex: uint8_t
    {
        DefaultDevice = 0xff,
        CordedDevice = 0,
        WirelessDevice1 = 1,
        WirelessDevice2 = 2,
        WirelessDevice3 = 3,
        WirelessDevice4 = 4,
        WirelessDevice5 = 5,
        WirelessDevice6 = 6,
    };

    static constexpr std::size_t ShortParamLength = 3;
    static constexpr std::size_t LongParamLength = 16;
} } }

#endif //LOGID_HIDPP_DEFS_H