/*
 * Copyright 2019-2020 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cassert>
#include <utility>
#include "../../util/thread.h"
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
    switch(_reason) {
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

std::string Device::devicePath() const
{
    return _path;
}

DeviceIndex Device::deviceIndex() const
{
    return _index;
}

std::tuple<uint8_t, uint8_t> Device::version() const
{
    return _version;
}

void Device::_init()
{
    _listening = false;
    _supported_reports = getSupportedReports(_raw_device->reportDescriptor());
    if(!_supported_reports)
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
    if(_listening)
        _raw_device->removeEventHandler("DEV_" + std::to_string(_index));
}

void Device::addEventHandler(const std::string& nickname,
        const std::shared_ptr<EventHandler>& handler)
{
    auto it = _event_handlers.find(nickname);
    assert(it == _event_handlers.end());

    _event_handlers.emplace(nickname, handler);
}

void Device::removeEventHandler(const std::string& nickname)
{
    _event_handlers.erase(nickname);
}

const std::map<std::string, std::shared_ptr<EventHandler>>&
    Device::eventHandlers()
{
    return _event_handlers;
}

void Device::handleEvent(Report& report)
{
    for(auto& handler : _event_handlers)
        if(handler.second->condition(report))
            handler.second->callback(report);
}

Report Device::sendReport(Report& report)
{
    switch(report.type())
    {
    case Report::Type::Short:
        if(!(_supported_reports & HIDPP_REPORT_SHORT_SUPPORTED))
            report.setType(Report::Type::Long);
        break;
    case Report::Type::Long:
        /* Report can be truncated, but that isn't a good idea. */
        assert(_supported_reports & HIDPP_REPORT_LONG_SUPPORTED);
    }

    auto raw_response = _raw_device->sendReport(report.rawReport());

    Report response(raw_response);

    Report::Hidpp10Error hidpp10_error{};
    if(response.isError10(&hidpp10_error))
        throw hidpp10::Error(hidpp10_error.error_code);

    Report::Hidpp20Error hidpp20_error{};
    if(response.isError20(&hidpp20_error))
        throw hidpp20::Error(hidpp20_error.error_code);

    return response;
}

std::string Device::name() const
{
    return _name;
}

uint16_t Device::pid() const
{
    return _pid;
}

void Device::listen()
{
    if(!_raw_device->isListening())
        thread::spawn({[raw=this->_raw_device]() {
            raw->listen();
        }});

    // Pass all HID++ events with device index to this device.
    auto handler = std::make_shared<raw::RawEventHandler>();
    handler->condition = [index=this->_index](std::vector<uint8_t>& report)
            ->bool {
        return (report[Offset::Type] == Report::Type::Short ||
                report[Offset::Type] == Report::Type::Long) &&
               (report[Offset::DeviceIndex] == index);
    };
    handler->callback = [this](std::vector<uint8_t>& report)->void {
        Report _report(report);
        this->handleEvent(_report);
    };

    _raw_device->addEventHandler("DEV_" + std::to_string(_index), handler);
    _listening = true;
}

void Device::stopListening()
{
    if(_listening)
        _raw_device->removeEventHandler("DEV_" + std::to_string(_index));

    _listening = false;

    if(!_raw_device->eventHandlers().empty())
        _raw_device->stopListener();
}