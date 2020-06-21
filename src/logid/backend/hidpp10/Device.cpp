#include <cassert>
#include <utility>
#include "Device.h"
#include "defs.h"

using namespace logid::backend;
using namespace logid::backend::hidpp10;

Device::Device(const std::string &path, hidpp::DeviceIndex index) :
    hidpp::Device(path, index)
{
    assert(version() == std::make_tuple(1, 0));
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_dev,
        hidpp::DeviceIndex index) : hidpp::Device(std::move(raw_dev), index)
{
    assert(version() == std::make_tuple(1, 0));
}

std::vector<uint8_t> Device::getRegister(uint8_t address,
        const std::vector<uint8_t>& params, hidpp::Report::Type type)
{
    assert(params.size() <= hidpp::LongParamLength);

    uint8_t sub_id = type == hidpp::Report::Type::Short ?
            GetRegisterShort : GetRegisterLong;

    return accessRegister(sub_id, address, params);
}

std::vector<uint8_t> Device::setRegister(uint8_t address,
                                         const std::vector<uint8_t>& params,
                                         hidpp::Report::Type type)
{
    assert(params.size() <= hidpp::LongParamLength);

    uint8_t sub_id = type == hidpp::Report::Type::Short ?
                     SetRegisterShort : SetRegisterLong;

    return accessRegister(sub_id, address, params);
}

std::vector<uint8_t> Device::accessRegister(uint8_t sub_id, uint8_t address, const std::vector<uint8_t> &params)
{
    hidpp::Report::Type type = params.size() <= hidpp::ShortParamLength ?
                               hidpp::Report::Type::Short : hidpp::Report::Type::Long;

    hidpp::Report request(type, deviceIndex(), sub_id, address);
    std::copy(params.begin(), params.end(), request.paramBegin());

    auto response = sendReport(request);
    return std::vector<uint8_t>(response.paramBegin(), response.paramEnd());
}



