#include "Feature.h"
#include "feature_defs.h"
#include "features/Root.h"

using namespace logid::backend::hidpp20;

const char* UnsupportedFeature::what() const noexcept
{
    return "Unsupported feature";
}

uint16_t UnsupportedFeature::code() const noexcept
{
    return _f_id;
}

std::vector<uint8_t> Feature::callFunction(uint8_t function_id, std::vector<uint8_t>& params)
{
    return _device->callFunction(_index, function_id, params);
}

Feature::Feature(Device* dev, uint16_t _id) : _device (dev)
{
    _index = hidpp20::FeatureID::ROOT;

    if(_id)
    {
        std::vector<uint8_t> getFunc_req(2);
        getFunc_req[0] = _id & 0xff;
        getFunc_req[1] = (_id >> 8) & 0xff;
        auto getFunc_resp = this->callFunction(Root::GetFeature, getFunc_req);
        _index = getFunc_resp[0];

        // 0 if not found
        if(!_index)
            throw UnsupportedFeature(_id);
    }
}
