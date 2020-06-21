#ifndef LOGID_BACKEND_HIDPP10_DEVICE_H
#define LOGID_BACKEND_HIDPP10_DEVICE_H

#include "../hidpp/Device.h"

namespace logid {
namespace backend {
namespace hidpp10
{
    class Device : public hidpp::Device
    {
    public:
        Device(const std::string& path, hidpp::DeviceIndex index);
        Device(std::shared_ptr<raw::RawDevice> raw_dev,
                hidpp::DeviceIndex index);

        std::vector<uint8_t> getRegister(uint8_t address,
                const std::vector<uint8_t>& params, hidpp::Report::Type type);

        std::vector<uint8_t> setRegister(uint8_t address,
                const std::vector<uint8_t>& params, hidpp::Report::Type type);
    private:
        std::vector<uint8_t> accessRegister(uint8_t sub_id,
                uint8_t address, const std::vector<uint8_t>& params);
    };
}}}

#endif //LOGID_BACKEND_HIDPP10_DEVICE_H