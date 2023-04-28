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

#ifndef LOGID_BACKEND_DJ_DEFS_H
#define LOGID_BACKEND_DJ_DEFS_H

#include <cstdint>

namespace logid::backend::dj {
    namespace ReportType {
        enum ReportType : uint8_t {
            Short = 0x20,
            Long = 0x21
        };
    }

    namespace DeviceType {
        enum DeviceType : uint8_t {
            Unknown = 0x00,
            Keyboard = 0x01,
            Mouse = 0x02,
            Numpad = 0x03,
            Presenter = 0x04,
            /* 0x05-0x07 is reserved */
            Trackball = 0x08,
            Touchpad = 0x09
        };
    }

    [[maybe_unused]]
    static constexpr uint8_t ErrorFeature = 0x7f;

    static constexpr std::size_t HeaderLength = 3;
    static constexpr std::size_t ShortParamLength = 12;
    static constexpr std::size_t LongParamLength = 29;
}

#endif //LOGID_BACKEND_DJ_DEFS_H