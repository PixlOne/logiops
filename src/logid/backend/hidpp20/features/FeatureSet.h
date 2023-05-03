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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_FEATURESET_H
#define LOGID_BACKEND_HIDPP20_FEATURE_FEATURESET_H

#include <backend/hidpp20/Feature.h>
#include <backend/hidpp20/feature_defs.h>
#include <map>

namespace logid::backend::hidpp20 {
    class FeatureSet : public Feature {
    public:
        static const uint16_t ID = FeatureID::FEATURE_SET;

        [[nodiscard]] uint16_t getID() final { return ID; }

        enum Function : uint8_t {
            GetFeatureCount = 0,
            GetFeature = 1
        };

        [[maybe_unused]] [[maybe_unused]]
        explicit FeatureSet(Device* device);

        [[nodiscard]] uint8_t getFeatureCount();

        [[nodiscard]] uint16_t getFeature(uint8_t feature_index);

        [[maybe_unused]]
        [[nodiscard]] std::map<uint8_t, uint16_t> getFeatures();
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_FEATURESET_H
