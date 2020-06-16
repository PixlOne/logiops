#include "Device.h"
#include "Report.h"

using namespace logid::backend;
using namespace logid::backend::hidpp;

const char* Device::InvalidDevice::what() const noexcept
{
    switch(_reason)
    {
        case NoHIDPPReport:
            return "Invalid HID++ device";
        case InvalidRawDevice:
            return "Invalid raw device";
    }
}

Device::InvalidDevice::Reason Device::InvalidDevice::code() const noexcept
{
    return _reason;
}

/// TODO: Initialize a single RawDevice for each path.
Device::Device(std::string path, DeviceIndex index):
    raw_device (std::make_shared<raw::RawDevice>(path)), path (path), index (index)
{
    supported_reports = getSupportedReports(raw_device->reportDescriptor());
    if(!supported_reports)
        throw InvalidDevice(InvalidDevice::NoHIDPPReport);
}