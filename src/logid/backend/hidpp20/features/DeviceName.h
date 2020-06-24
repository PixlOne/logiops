#ifndef LOGID_BACKEND_HIDPP20_FEATURE_DEVICENAME_H
#define LOGID_BACKEND_HIDPP20_FEATURE_DEVICENAME_H

#include "../Feature.h"
#include "../feature_defs.h"
#include "../EssentialFeature.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class DeviceName : public Feature
    {
    public:
        static const uint16_t ID = FeatureID::DEVICE_NAME;
        virtual uint16_t getID() { return ID; }

        enum Function : uint8_t
        {
            GetLength = 0,
            GetDeviceName = 1
        };

        explicit DeviceName(Device* device);

        uint8_t getNameLength();
        std::string getName();
    };

    class EssentialDeviceName : public EssentialFeature
    {
    public:
        static const uint16_t ID = FeatureID::DEVICE_NAME;
        virtual uint16_t getID() { return ID; }

        explicit EssentialDeviceName(hidpp::Device* device);

        uint8_t getNameLength();
        std::string getName();
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_DEVICENAME_H