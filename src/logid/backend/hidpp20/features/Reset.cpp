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
#include <backend/hidpp20/features/Reset.h>

using namespace logid::backend::hidpp20;

Reset::Reset(Device* device) : Feature(device, ID) {
}

uint16_t Reset::getProfile() {
    std::vector<uint8_t> params(0);
    auto results = callFunction(GetProfile, params);

    uint16_t profile = results[1];
    profile |= (results[0] << 8);
    return profile;
}

void Reset::reset(uint16_t profile) {
    std::vector<uint8_t> params(2);
    params[0] = (profile >> 8) & 0xff;
    params[1] = profile & 0xff;
    callFunction(ResetToProfile, params);
}