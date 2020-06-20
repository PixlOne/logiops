#ifndef LOGID_BACKEND_DJ_DEFS_H
#define LOGID_BACKEND_DJ_DEFS_H

#include <cstdint>

namespace logid {
namespace backend {
namespace dj
{
    namespace ReportType
    {
        enum ReportType : uint8_t
        {
            Short = 0x20,
            Long = 0x21
        };
    }

    namespace DeviceType
    {
        enum DeviceType : uint8_t
        {
            Unknown = 0x00,
            Keyboard = 0x01,
            Mouse = 0x02,
            Numpad = 0x03,
            Presenter = 0x04,
            /* 0x05-0x07 is reserved */
            Trackball = 0x08,
            Touchpad = 0x09
        };
    }

    static constexpr uint8_t ErrorFeature = 0x7f;

    static constexpr std::size_t HeaderLength = 3;
    static constexpr std::size_t ShortParamLength = 12;
    static constexpr std::size_t LongParamLength = 29;
}}}

#endif //LOGID_BACKEND_DJ_DEFS_H