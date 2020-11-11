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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_FEATURESET_H
#define LOGID_BACKEND_HIDPP20_FEATURE_FEATURESET_H

#include "../Feature.h"
#include "../feature_defs.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class FeatureSet : public Feature
    {
    public:
        static const uint16_t ID = FeatureID::FEATURE_SET;
        virtual uint16_t getID() { return ID; }

        enum Function : uint8_t
        {
            GetFeatureCount = 0,
            GetFeature = 1
        };

        explicit FeatureSet(Device* device);

        uint8_t getFeatureCount();
        uint16_t getFeature(uint8_t feature_index);
        std::map<uint8_t, uint16_t> getFeatures();
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_FEATURESET_H
