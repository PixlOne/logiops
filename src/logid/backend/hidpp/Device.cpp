#include <cassert>
#include <utility>
#include "Device.h"
#include "Report.h"
#include "../hidpp20/features/Root.h"
#include "../hidpp20/features/DeviceName.h"
#include "../hidpp20/Error.h"
#include "../hidpp10/Error.h"
#include "../dj/Receiver.h"

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
        case Asleep:
            return "Device asleep";
        default:
            return "Invalid device";
    }
}

Device::InvalidDevice::Reason Device::InvalidDevice::code() const noexcept
{
    return _reason;
}

/// TODO: Initialize a single RawDevice for each path.
Device::Device(const std::string& path, DeviceIndex index):
    _raw_device (std::make_shared<raw::RawDevice>(path)),  _receiver (nullptr),
    _path (path), _index (index)
{
    _init();
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_device, DeviceIndex index) :
    _raw_device (std::move(raw_device)), _receiver (nullptr),
    _path (_raw_device->hidrawPath()), _index (index)
{
    _init();
}

Device::Device(std::shared_ptr<dj::Receiver> receiver,
        hidpp::DeviceConnectionEvent event) :
    _raw_device (receiver->rawDevice()), _index (event.index)
{
    // Device will throw an error soon, just do it now
    if(!event.linkEstablished)
        throw InvalidDevice(InvalidDevice::Asleep);

    _pid = event.pid;
    _init();
}

void Device::_init()
{
    supported_reports = getSupportedReports(_raw_device->reportDescriptor());
    if(!supported_reports)
        throw InvalidDevice(InvalidDevice::NoHIDPPReport);

    try {
        hidpp20::EssentialRoot root(this);
        _version = root.getVersion();
    } catch(hidpp10::Error &e) {
        // Valid HID++ 1.0 devices should send an InvalidSubID error
        if(e.code() != hidpp10::Error::InvalidSubID)
            throw;

        // HID++ 2.0 is not supported, assume HID++ 1.0
        _version = std::make_tuple(1, 0);
    }

    if(!_receiver) {
        _pid = _raw_device->productId();
        if(std::get<0>(_version) >= 2) {
            try {
                hidpp20::EssentialDeviceName deviceName(this);
                _name = deviceName.getName();
            } catch(hidpp20::UnsupportedFeature &e) {
                _name = _raw_device->name();
            }
        } else {
            _name = _raw_device->name();
        }
    } else {
        _name = _receiver->getDeviceName(_index);
    }
}

Device::~Device()
{
    _raw_device->removeEventHandler("DEV_" + std::to_string(_index));
}

void Device::addEventHandler(const std::string& nickname,
        const std::shared_ptr<EventHandler>& handler)
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

    auto raw_response = _raw_device->sendReport(report.rawReport());

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
    if(!_raw_device->isListening())
        std::thread{[=]() { _raw_device->listen(); }}.detach();

    // Pass all HID++ events with device index to this device.
    std::shared_ptr<RawEventHandler> rawEventHandler;
    rawEventHandler->condition = [this](std::vector<uint8_t>& report)->bool
    {
        return (report[Offset::Type] == Report::Type::Short ||
                report[Offset::Type] == Report::Type::Long) &&
               (report[Offset::DeviceIndex] == this->_index);
    };
    rawEventHandler->callback = [this](std::vector<uint8_t>& report)->void
    {
        Report _report(report);
        this->handleEvent(_report);
    };

    _raw_device->addEventHandler("DEV_" + std::to_string(_index), rawEventHandler);
}

void Device::stopListening()
{
    _raw_device->removeEventHandler("DEV_" + std::to_string(_index));

    if(!_raw_device->eventHandlers().empty())
        _raw_device->stopListener();
}