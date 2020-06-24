#include "Root.h"

using namespace logid::backend::hidpp20;

Root::Root(Device* dev) : Feature(dev, ID)
{
}

std::vector<uint8_t> _genGetFeatureParams(uint16_t feature_id)
{
    std::vector<uint8_t> params(2);
    params[0] = feature_id & 0xff;
    params[1] = (feature_id >> 8) & 0xff;
    return params;
}

feature_info _genGetFeatureInfo(uint16_t feature_id,
        std::vector<uint8_t> response)
{
    feature_info info{};
    info.feature_id = response[0];

    if(!info.feature_id)
        throw UnsupportedFeature(feature_id);

    info.hidden = response[1] & Root::FeatureFlag::Hidden;
    info.obsolete = response[1] & Root::FeatureFlag::Obsolete;
    info.internal = response[1] & Root::FeatureFlag::Internal;

    return info;
}

feature_info Root::getFeature(uint16_t feature_id)
{
    auto params = _genGetFeatureParams(feature_id);
    auto response = this->callFunction(Function::GetFeature, params);
    return _genGetFeatureInfo(feature_id, response);
}

std::tuple<uint8_t, uint8_t> Root::getVersion()
{
    std::vector<uint8_t> params(0);
    auto response = this->callFunction(Function::Ping, params);

    return std::make_tuple(response[0], response[1]);
}

EssentialRoot::EssentialRoot(hidpp::Device* dev) : EssentialFeature(dev, ID)
{
}

feature_info EssentialRoot::getFeature(uint16_t feature_id)
{
    auto params = _genGetFeatureParams(feature_id);
    auto response = this->callFunction(Root::Function::GetFeature, params);
    return _genGetFeatureInfo(feature_id, response);
}

std::tuple<uint8_t, uint8_t> EssentialRoot::getVersion()
{
    std::vector<uint8_t> params(0);
    auto response = this->callFunction(Root::Function::Ping, params);

    return std::make_tuple(response[0], response[1]);
}