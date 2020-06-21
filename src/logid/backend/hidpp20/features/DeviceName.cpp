#include <cmath>
#include "DeviceName.h"

using namespace logid::backend;
using namespace logid::backend::hidpp20;

DeviceName::DeviceName(Device* dev) : Feature(dev, ID)
{
}

uint8_t DeviceName::getNameLength()
{
    std::vector<uint8_t> params(0);

    auto response = this->callFunction(Function::GetLength, params);
    return response[0];
}

std::string _getName(uint8_t length,
        const std::function<std::vector<uint8_t>(std::vector<uint8_t>)>& fcall)
{
    uint8_t function_calls = length/hidpp::LongParamLength;
    if(length % hidpp::LongParamLength)
        function_calls++;
    std::vector<uint8_t> params(1);
    std::string name;

    for(uint8_t i = 0; i < function_calls; i++) {
        params[0] = i*hidpp::LongParamLength;
        auto name_section = fcall(params);
        for(std::size_t j = 0; j < hidpp::LongParamLength; j++) {
            if(params[0] + j >= length)
                return name;
            name += name_section[j];
        }
    }

    return name;
}

std::string DeviceName::getName()
{
    return _getName(getNameLength(), [this]
    (std::vector<uint8_t> params)->std::vector<uint8_t> {
        return this->callFunction(Function::GetDeviceName, params);
    });
}

EssentialDeviceName::EssentialDeviceName(hidpp::Device* dev) :
    EssentialFeature(dev, ID)
{
}

uint8_t EssentialDeviceName::getNameLength()
{
    std::vector<uint8_t> params(0);

    auto response = this->callFunction(DeviceName::Function::GetLength, params);
    return response[0];
}

std::string EssentialDeviceName::getName()
{
    return _getName(getNameLength(), [this]
    (std::vector<uint8_t> params)->std::vector<uint8_t> {
        return this->callFunction(DeviceName::Function::GetDeviceName, params);
    });
}