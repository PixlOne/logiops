#ifndef LOGID_HIDPP_DEVICE_H
#define LOGID_HIDPP_DEVICE_H

#include <string>
#include <memory>
#include "../raw/RawDevice.h"

namespace logid::backend::hidpp
{
    enum DeviceIndex: uint8_t
    {
        DefaultDevice = 0,
        WirelessDevice1 = 1,
        WirelessDevice2 = 2,
        WirelessDevice3 = 3,
        WirelessDevice4 = 4,
        WirelessDevice5 = 5,
        WirelessDevice6 = 6,
        CordedDevice = 0xff
    };

    class Device
    {
    public:
        class InvalidDevice : std::exception
        {
        public:
            enum Reason
            {
                NoHIDPPReport,
                InvalidRawDevice
            };
            InvalidDevice(Reason reason) : _reason (reason) {}
            virtual const char *what() const noexcept;
            virtual Reason code() const noexcept;
        private:
            Reason _reason;

        };
        Device(std::string path, DeviceIndex index);
        std::string devicePath() const { return path; }
        DeviceIndex deviceIndex() const { return index; }
    private:
        std::shared_ptr<logid::backend::raw::RawDevice> raw_device;
        std::string path;
        DeviceIndex index;
        uint8_t supported_reports;
    };
}

#endif //LOGID_HIDPP_DEVICE_H