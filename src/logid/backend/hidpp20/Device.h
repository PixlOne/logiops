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
        std::vector<uint8_t> callFunction(uint8_t feature_index,
                uint8_t function,
                std::vector<uint8_t>& params);
    };
}}}

#endif //LOGID_HIDPP20_DEVICE_H