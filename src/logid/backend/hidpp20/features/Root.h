#ifndef LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H
#define LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H

#include "../Feature.h"
#include "../EssentialFeature.h"
#include "../feature_defs.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class Root : public Feature
    {
    public:
        static const uint16_t ID = FeatureID::ROOT;
        virtual uint16_t getID() { return ID; }

        enum Function : uint8_t
        {
            GetFeature = 0,
            Ping = 1
        };

        explicit Root(Device* device);

        feature_info getFeature (uint16_t feature_id);
        std::tuple<uint8_t, uint8_t> getVersion();

        enum FeatureFlag : uint8_t
        {
            Obsolete = 1<<7,
            Hidden = 1<<6,
            Internal = 1<<5
        };
    };

    class EssentialRoot : public EssentialFeature
    {
    public:
        static const uint16_t ID = FeatureID::ROOT;
        virtual uint16_t getID() { return ID; }

        explicit EssentialRoot(hidpp::Device* device);

        feature_info getFeature(uint16_t feature_id);
        std::tuple<uint8_t, uint8_t> getVersion();
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H