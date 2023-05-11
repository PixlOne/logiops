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

#ifndef LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H
#define LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H

#include <backend/hidpp20/EssentialFeature.h>
#include <backend/hidpp20/feature_defs.h>

namespace logid::backend::hidpp20 {

    class Root : public EssentialFeature {
    public:
        static const uint16_t ID = FeatureID::ROOT;

        uint16_t getID() final { return ID; }

        explicit Root(hidpp::Device* device);

        enum Function : uint8_t {
            GetFeature = 0,
            Ping = 1
        };

        feature_info getFeature(uint16_t feature_id);

        uint8_t ping(uint8_t byte);

        std::tuple<uint8_t, uint8_t> getVersion();

        enum FeatureFlag : uint8_t {
            Obsolete = 1 << 7,
            Hidden = 1 << 6,
            Internal = 1 << 5
        };
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H