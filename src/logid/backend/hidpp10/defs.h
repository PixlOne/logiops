#ifndef LOGID_BACKEND_HIDPP10_DEFS_H
#define LOGID_BACKEND_HIDPP10_DEFS_H

namespace logid {
namespace backend {
namespace hidpp10
{
    enum SubID: uint8_t
    {
        SetRegisterShort = 0x80,
        GetRegisterShort = 0x81,
        SetRegisterLong = 0x82,
        GetRegisterLong = 0x83
    };
}}}

#endif //LOGID_BACKEND_HIDPP10_DEFS_H