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
#include <backend/hidpp20/features/FeatureSet.h>

using namespace logid::backend::hidpp20;

[[maybe_unused]]
FeatureSet::FeatureSet(Device* device) : Feature(device, ID) {
}

uint8_t FeatureSet::getFeatureCount() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetFeatureCount, params);
    return response[0];
}

uint16_t FeatureSet::getFeature(uint8_t feature_index) {
    std::vector<uint8_t> params(1);
    params[0] = feature_index;
    auto response = callFunction(GetFeature, params);

    uint16_t feature_id = (response[0] << 8);
    feature_id |= response[1];
    return feature_id;
}

[[maybe_unused]]
std::map<uint8_t, uint16_t> FeatureSet::getFeatures() {
    uint8_t feature_count = getFeatureCount();
    std::map<uint8_t, uint16_t> features;
    for (uint8_t i = 0; i < feature_count; i++)
        features[i] = getFeature(i);
    return features;
}