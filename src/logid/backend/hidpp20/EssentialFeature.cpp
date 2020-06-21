#include <cassert>
#include "EssentialFeature.h"
#include "feature_defs.h"
#include "features/Root.h"

using namespace logid::backend::hidpp20;

std::vector<uint8_t> EssentialFeature::callFunction(uint8_t function_id,
        std::vector<uint8_t>& params)
{
    hidpp::Report::Type type;

    assert(params.size() <= hidpp::LongParamLength);
    if(params.size() <= hidpp::ShortParamLength)
        type = hidpp::Report::Type::Short;
    else if(params.size() <= hidpp::LongParamLength)
        type = hidpp::Report::Type::Long;

    hidpp::Report request(type, _device->deviceIndex(), _index, function_id,
                          LOGID_HIDPP_SOFTWARE_ID);
    std::copy(params.begin(), params.end(), request.paramBegin());

    auto response = _device->sendReport(request);
    return std::vector<uint8_t>(response.paramBegin(), response.paramEnd());
}

EssentialFeature::EssentialFeature(hidpp::Device* dev, uint16_t _id) :
    _device (dev)
{
    _index = hidpp20::FeatureID::ROOT;

    if(_id)
    {
        std::vector<uint8_t> getFunc_req(2);
        getFunc_req[0] = (_id >> 8) & 0xff;
        getFunc_req[1] = _id & 0xff;
        auto getFunc_resp = this->callFunction(Root::GetFeature, getFunc_req);
        _index = getFunc_resp[0];

        // 0 if not found
        if(!_index)
            throw UnsupportedFeature(_id);
    }
}
