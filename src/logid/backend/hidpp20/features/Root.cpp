#include "Root.h"

using namespace logid::backend::hidpp20;

Root::Root(Device* dev) : Feature(dev, ID)
{
}

feature_info Root::getFeature(uint16_t feature_id)
{
    feature_info info{};
    std::vector<uint8_t> params(2);
    params[0] = feature_id & 0xff;
    params[1] = (feature_id >> 8) & 0xff;

    auto response = this->callFunction(Function::GetFeature, params);

    info.feature_id = response[0];

    if(!info.feature_id)
        throw UnsupportedFeature(feature_id);

    info.hidden = response[1] & FeatureFlag::Hidden;
    info.obsolete = response[1] & FeatureFlag::Obsolete;
    info.internal = response[1] & FeatureFlag::Internal;

    return info;
}

std::tuple<uint8_t, uint8_t> Root::getVersion()
{
    std::vector<uint8_t> params(0);
    auto response = this->callFunction(Function::Ping, params);

    return std::make_tuple(response[0], response[1]);
}