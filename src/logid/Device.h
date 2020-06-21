#ifndef LOGID_DEVICE_H
#define LOGID_DEVICE_H

#include "backend/hidpp/defs.h"
#include "backend/hidpp20/Device.h"

namespace logid
{
    /* TODO: Implement HID++ 1.0 support
     * Currently, the logid::Device class has a hardcoded requirement
     * for an HID++ 2.0 device.
     */
    class Device
    {
    public:
        Device(std::string path, backend::hidpp::DeviceIndex index);
        Device(const std::shared_ptr<backend::raw::RawDevice>& raw_device,
                backend::hidpp::DeviceIndex index);
        void wakeup();
        void sleep();
    private:
        backend::hidpp20::Device _hidpp20;
        std::string _path;
        backend::hidpp::DeviceIndex _index;
    };
}

#endif //LOGID_DEVICE_H
