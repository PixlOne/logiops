#ifndef LOGID_HIDPP_DEFS_H
#define LOGID_HIDPP_DEFS_H

#define LOGID_HIDPP_SOFTWARE_ID 0

namespace logid::backend::hidpp
{
    namespace ReportType
    {
        enum ReportType : uint8_t
        {
            Short = 0x10,
            Long = 0x11
        };
    }

    enum DeviceIndex: uint8_t
    {
        DefaultDevice = 0xff,
        CordedDevice = 0,
        WirelessDevice1 = 1,
        WirelessDevice2 = 2,
        WirelessDevice3 = 3,
        WirelessDevice4 = 4,
        WirelessDevice5 = 5,
        WirelessDevice6 = 6,
    };

    static constexpr std::size_t ShortParamLength = 3;
    static constexpr std::size_t LongParamLength = 16;
}

#endif //LOGID_HIDPP_DEFS_H