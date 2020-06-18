#include <assert.h>
#include "Device.h"
#include "Report.h"
#include "../hidpp20/features/Root.h"

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
Device::Device(const std::string& path, DeviceIndex index):
    raw_device (std::make_shared<raw::RawDevice>(path)), path (path), index (index)
{
    supported_reports = getSupportedReports(raw_device->reportDescriptor());
    if(!supported_reports)
        throw InvalidDevice(InvalidDevice::NoHIDPPReport);

    Report versionRequest(Report::Type::Short, index, hidpp20::FeatureID::ROOT,
            hidpp20::Root::Ping, LOGID_HIDPP_SOFTWARE_ID);

    ///TODO: Catch error
    auto versionResponse = sendReport(versionRequest);
    auto versionResponse_params = versionResponse.paramBegin();
    _version = std::make_tuple(versionResponse_params[0], versionResponse_params[1]);

    // Pass all HID++ events with device index to this device.
    RawEventHandler rawEventHandler;
    rawEventHandler.condition = [index](std::vector<uint8_t>& report)->bool
    {
        return (report[Offset::Type] == Report::Type::Short ||
            report[Offset::Type] == Report::Type::Long) && (report[Offset::DeviceIndex] == index);
    };
    rawEventHandler.callback = [this](std::vector<uint8_t>& report)->void
    {
        Report _report(report);
        this->handleEvent(_report);
    };

    raw_device->addEventHandler("DEV_" + std::to_string(index), rawEventHandler);
}

void Device::addEventHandler(const std::string& nickname, const std::shared_ptr<EventHandler>& handler)
{
    auto it = event_handlers.find(nickname);
    assert(it == event_handlers.end());

    event_handlers.emplace(nickname, handler);
}

void Device::removeEventHandler(const std::string& nickname)
{
    event_handlers.erase(nickname);
}

void Device::handleEvent(Report& report)
{
    for(auto& handler : event_handlers)
        if(handler.second->condition(report))
            handler.second->callback(report);
}

Report Device::sendReport(Report& report)
{
    switch(report.type())
    {
        case Report::Type::Short:
            if(!(supported_reports & HIDPP_REPORT_SHORT_SUPPORTED))
                report.setType(Report::Type::Long);
            break;
        case Report::Type::Long:
            /* Report can be truncated, but that isn't a good idea. */
            assert(supported_reports & HIDPP_REPORT_LONG_SUPPORTED);
    }

    auto raw_response = raw_device->sendReport(report.rawReport());
    return Report(raw_response);
}

void Device::listen()
{
    raw_device->listen();
}
