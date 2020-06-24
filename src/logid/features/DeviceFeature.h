#ifndef LOGID_FEATURES_DEVICEFEATURE_H
#define LOGID_FEATURES_DEVICEFEATURE_H

namespace logid {
namespace features
{
    class DeviceFeature
    {
    public:
        virtual void configure() = 0;
        virtual void listen() = 0;
    };
}}

#endif //LOGID_FEATURES_DEVICEFEATURE_H
