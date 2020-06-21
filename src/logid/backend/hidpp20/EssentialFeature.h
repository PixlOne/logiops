#ifndef LOGID_HIDPP20_ESSENTIAL_FEATURE_H
#define LOGID_HIDPP20_ESSENTIAL_FEATURE_H

// WARNING: UNSAFE

/* This class is only meant to provide essential HID++ 2.0 features to the
 * hidpp::Device class. No version checks are provided here
 */

#include "Device.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class EssentialFeature
    {
    public:
        static const uint16_t ID;
        virtual uint16_t getID() = 0;

    protected:
        EssentialFeature(hidpp::Device* dev, uint16_t _id);
        std::vector<uint8_t> callFunction(uint8_t function_id,
                std::vector<uint8_t>& params);
    private:
        hidpp::Device* _device;
        uint8_t _index;
    };
}}}

#endif //LOGID_HIDPP20_ESSENTIAL_FEATURE_H