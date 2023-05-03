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

#ifndef LOGID_BACKEND_HIDPP10_DEFS_H
#define LOGID_BACKEND_HIDPP10_DEFS_H

namespace logid::backend::hidpp10 {
    enum SubID : uint8_t {
        SetRegisterShort = 0x80,
        GetRegisterShort = 0x81,
        SetRegisterLong = 0x82,
        GetRegisterLong = 0x83,
    };

    static constexpr size_t SubIDCount = 4;
}

#endif //LOGID_BACKEND_HIDPP10_DEFS_H