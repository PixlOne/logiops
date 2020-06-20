#ifndef LOGID_HIDPP20_DEVICE_H
#define LOGID_HIDPP20_DEVICE_H

#include "../hidpp/Device.h"
#include <cstdint>

namespace logid {
namespace backend {
namespace hidpp20 {
    class Device : public hidpp::Device
    {
    public:
        Device(std::string path, hidpp::DeviceIndex index);
        Device(std::shared_ptr<raw::RawDevice> raw_device, hidpp::DeviceIndex index);

        std::vector<uint8_t> callFunction(uint8_t feature_index,
                uint8_t function,
                std::vector<uint8_t>& params);
    };
}}}

#endif //LOGID_HIDPP20_DEVICE_H