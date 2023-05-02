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

#ifndef LOGID_BACKEND_HIDPP20_ESSENTIAL_FEATURE_H
#define LOGID_BACKEND_HIDPP20_ESSENTIAL_FEATURE_H

// WARNING: UNSAFE

/* This class is only meant to provide essential HID++ 2.0 features to the
 * hidpp::Device class. No version checks are provided here
 */

#include <backend/hidpp/Device.h>

namespace logid::backend::hidpp20 {
    class EssentialFeature {
    public:
        static const uint16_t ID;

        virtual uint16_t getID() = 0;

    protected:
        EssentialFeature(hidpp::Device* dev, uint16_t _id);

        std::vector<uint8_t> callFunction(uint8_t function_id,
                                          std::vector<uint8_t>& params);

        hidpp::Device* const _device;
        uint8_t _index;
    };
}

#endif //LOGID_BACKEND_HIDPP20_ESSENTIAL_FEATURE_H