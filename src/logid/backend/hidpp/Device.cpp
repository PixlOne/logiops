#include <assert.h>
#include "Device.h"
#include "Report.h"
#include "../hidpp20/features/Root.h"
#include "../hidpp20/Error.h"
#include "../hidpp10/Error.h"

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
    raw_device (std::make_shared<raw::RawDevice>(path)), path (path),
    _index (index)
{
    _init();
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_device, DeviceIndex index) :
    raw_device (raw_device), _index (index)
{
    _init();
}

void Device::_init()
{
    supported_reports = getSupportedReports(raw_device->reportDescriptor());
    if(!supported_reports)
        throw InvalidDevice(InvalidDevice::NoHIDPPReport);

    try
    {
        Report versionRequest(Report::Type::Short, _index,
                hidpp20::FeatureID::ROOT,hidpp20::Root::Ping,
                LOGID_HIDPP_SOFTWARE_ID);

        auto versionResponse = sendReport(versionRequest);
        auto versionResponse_params = versionResponse.paramBegin();
        _version = std::make_tuple(versionResponse_params[0], versionResponse_params[1]);
    }
    catch(hidpp10::Error &e)
    {
        // Valid HID++ 1.0 devices should send an InvalidSubID error
        if(e.code() != hidpp10::Error::InvalidSubID)
            throw;

        // HID++ 2.0 is not supported, assume HID++ 1.0
        _version = std::make_tuple(1, 0);
    }

    // Pass all HID++ events with device index to this device.
    RawEventHandler rawEventHandler;
    rawEventHandler.condition = [this](std::vector<uint8_t>& report)->bool
    {
        return (report[Offset::Type] == Report::Type::Short ||
                report[Offset::Type] == Report::Type::Long) &&
                (report[Offset::DeviceIndex] == this->_index);
    };
    rawEventHandler.callback = [this](std::vector<uint8_t>& report)->void
    {
        Report _report(report);
        this->handleEvent(_report);
    };

    raw_device->addEventHandler("DEV_" + std::to_string(_index), rawEventHandler);
}

Device::~Device()
{
    raw_device->removeEventHandler("DEV_" + std::to_string(_index));
    ///TODO: tmp
    raw_device->stopListener();
    raw_device.reset();
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

    Report response(raw_response);

    Report::hidpp10_error hidpp10Error{};
    if(response.isError10(&hidpp10Error))
        throw hidpp10::Error(hidpp10Error.error_code);

    Report::hidpp20_error hidpp20Error{};
    if(response.isError20(&hidpp20Error))
        throw hidpp10::Error(hidpp20Error.error_code);

    return response;
}

void Device::listen()
{
    raw_device->listen();
}
