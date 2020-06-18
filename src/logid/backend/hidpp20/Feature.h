#ifndef LOGID_HIDPP20_FEATURE_H
#define LOGID_HIDPP20_FEATURE_H

#include <cstdint>
#include "Device.h"

namespace logid {
namespace backend {
namespace hidpp20 {
    class UnsupportedFeature : public std::exception
    {
    public:
        explicit UnsupportedFeature(uint16_t ID) : _f_id (ID) {}
        virtual const char* what() const noexcept;
        uint16_t code() const noexcept;
    private:
        uint16_t _f_id;
    };

    class Feature
    {
    public:
        static const uint16_t ID;
        virtual uint16_t getID() = 0;

    protected:
        explicit Feature(Device* dev, uint16_t _id);
        std::vector<uint8_t> callFunction(uint8_t function_id,
            std::vector<uint8_t>& params);
    private:
        Device* _device;
        uint8_t _index;
    };
}}}

#endif //LOGID_HIDPP20_FEATURE_H