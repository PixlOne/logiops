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

#include <backend/hidpp20/features/Root.h>
#include <backend/hidpp20/Feature.h>
#include <backend/hidpp20/Error.h>

using namespace logid::backend::hidpp20;

namespace {
    std::vector<uint8_t> _genGetFeatureParams(uint16_t feature_id) {
        std::vector<uint8_t> params(2);
        params[0] = feature_id & 0xff;
        params[1] = (feature_id >> 8) & 0xff;
        return params;
    }

    feature_info _genGetFeatureInfo(uint16_t feature_id,
                                    std::vector<uint8_t> response) {
        feature_info info{};
        info.feature_id = response[0];

        if (!info.feature_id)
            throw UnsupportedFeature(feature_id);

        info.hidden = response[1] & Root::FeatureFlag::Hidden;
        info.obsolete = response[1] & Root::FeatureFlag::Obsolete;
        info.internal = response[1] & Root::FeatureFlag::Internal;

        return info;
    }
}

Root::Root(hidpp::Device* dev) : EssentialFeature(dev, ID) {
}

feature_info Root::getFeature(uint16_t feature_id) {
    auto params = _genGetFeatureParams(feature_id);
    try {
        auto response = this->callFunction(Root::Function::GetFeature, params);
        return _genGetFeatureInfo(feature_id, response);
    } catch (Error& e) {
        if (e.code() == Error::InvalidFeatureIndex)
            throw UnsupportedFeature(feature_id);
        throw e;
    }
}

uint8_t Root::ping(uint8_t byte) {
    std::vector<uint8_t> params(3);
    params[2] = byte;

    auto response = this->callFunction(Root::Function::Ping, params);

    return response[2];
}

std::tuple<uint8_t, uint8_t> Root::getVersion() {
    std::vector<uint8_t> params(0);
    auto response = this->callFunction(Root::Function::Ping, params);

    if (response[0] == 0x11)
        return std::make_tuple(1, 0);

    return std::make_tuple(response[0], response[1]);
}