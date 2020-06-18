#include "Feature.h"

using namespace logid::backend::hidpp20;

const char* Feature::UnsupportedFeature::what() const noexcept
{
    return "Unsupported feature";
}

uint16_t Feature::UnsupportedFeature::code() const noexcept
{
    return _f_id;
}

std::vector<uint8_t> Feature::callFunction(uint8_t function_id, std::vector<uint8_t>& params)
{
    return _device->callFunction(_index, function_id, params);
}

Feature::Feature(Device* dev, uint16_t _id) : _device (dev), _index (0xff)
{
    ///TODO: Set index
}

Feature::Feature(Device* dev, uint8_t _index) : _device (dev), _index (_index)
{

}
