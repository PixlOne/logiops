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

#include <backend/hidpp20/Feature.h>
#include <backend/hidpp20/EssentialFeature.h>
#include <backend/hidpp20/features/Root.h>
#include <backend/hidpp20/Error.h>
#include <cassert>

using namespace logid::backend::hidpp20;

std::vector<uint8_t> EssentialFeature::callFunction(uint8_t function_id,
                                                    std::vector<uint8_t>& params) {
    hidpp::Report::Type type;

    assert(params.size() <= hidpp::LongParamLength);
    if (params.size() <= hidpp::ShortParamLength)
        type = hidpp::Report::Type::Short;
    else if (params.size() <= hidpp::LongParamLength)
        type = hidpp::Report::Type::Long;
    else
        throw hidpp::Report::InvalidReportID();

    hidpp::Report request(type, _device->deviceIndex(), _index, function_id, hidpp::softwareID);
    std::copy(params.begin(), params.end(), request.paramBegin());

    auto response = _device->sendReport(request);
    return {response.paramBegin(), response.paramEnd()};
}

EssentialFeature::EssentialFeature(hidpp::Device* dev, uint16_t _id) :
        _device(dev) {
    _index = hidpp20::FeatureID::ROOT;

    if (_id) {
        std::vector<uint8_t> getFunc_req(2);
        getFunc_req[0] = (_id >> 8) & 0xff;
        getFunc_req[1] = _id & 0xff;
        try {
            _index = this->callFunction(Root::GetFeature, getFunc_req).at(0);
        } catch (Error& e) {
            if (e.code() == Error::InvalidFeatureIndex)
                throw UnsupportedFeature(_id);
            throw e;
        }

        // 0 if not found
        if (!_index)
            throw UnsupportedFeature(_id);
    }
}
