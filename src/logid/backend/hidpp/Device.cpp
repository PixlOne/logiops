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
#include "Device.h"
#include "Report.h"
#include "../hidpp20/features/Root.h"
#include "../hidpp20/features/DeviceName.h"
#include "../hidpp20/Error.h"
#include "../hidpp10/Error.h"
#include "../Error.h"
#include "../dj/Receiver.h"

using namespace logid::backend;
using namespace logid::backend::hidpp;

using namespace std::chrono;

const char* Device::InvalidDevice::what() const noexcept
{
    switch(_reason) {
    case NoHIDPPReport:
        return "Invalid HID++ device";
    case InvalidRawDevice:
        return "Invalid raw device";
    case Asleep:
        return "Device asleep";
    case VirtualNode:
        return "Virtual device";
    default:
        return "Invalid device";
    }
}

Device::InvalidDevice::Reason Device::InvalidDevice::code() const noexcept
{
    return _reason;
}

Device::Device(const std::string& path, DeviceIndex index,
               std::shared_ptr<raw::DeviceMonitor> monitor, double timeout):
    io_timeout (duration_cast<milliseconds>(
            duration<double, std::milli>(timeout))),
    _raw_device (std::make_shared<raw::RawDevice>(path, std::move(monitor))),
    _receiver (nullptr), _path (path), _index (index)
{
    _init();
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_device, DeviceIndex index,
               double timeout) :
        io_timeout (duration_cast<milliseconds>(
                duration<double, std::milli>(timeout))),
        _raw_device (std::move(raw_device)), _receiver (nullptr),
        _path (_raw_device->rawPath()), _index (index)
{
    _init();
}

Device::Device(std::shared_ptr<dj::Receiver> receiver,
        hidpp::DeviceConnectionEvent event, double timeout) :
        io_timeout (duration_cast<milliseconds>(
                duration<double, std::milli>(timeout))),
        _raw_device (receiver->rawDevice()), _receiver (receiver),
        _path (receiver->rawDevice()->rawPath()), _index (event.index)
{
    // Device will throw an error soon, just do it now
    if(!event.linkEstablished)
        throw InvalidDevice(InvalidDevice::Asleep);

    if(!event.fromTimeoutCheck)
        _pid = event.pid;
    else
        _pid = receiver->getPairingInfo(_index).pid;
    _init();
}

Device::Device(std::shared_ptr<dj::Receiver> receiver,
        DeviceIndex index, double timeout) :
        io_timeout (duration_cast<milliseconds>(
                duration<double, std::milli>(timeout))),
        _raw_device (receiver->rawDevice()),
        _receiver (receiver), _path (receiver->rawDevice()->rawPath()),
        _index (index)
{
    _pid = receiver->getPairingInfo(_index).pid;
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
    supported_reports = getSupportedReports(_raw_device->reportDescriptor());
    if(!supported_reports)
        throw InvalidDevice(InvalidDevice::NoHIDPPReport);

    /* hid_logitech_dj creates virtual /dev/hidraw nodes for receiver
     * devices. We should ignore these devices.
     */
    if(_index == hidpp::DefaultDevice) {
        _raw_handler = _raw_device->addEventHandler({
                [index=this->_index](const std::vector<uint8_t>& report)->bool {
                    return (report[Offset::Type] == Report::Type::Short ||
                            report[Offset::Type] == Report::Type::Long);
                }, [this](const std::vector<uint8_t>& report)->void {
                    Report _report(report);
                    this->handleEvent(_report);
        } });

        try {
            auto rsp = sendReport(
                    {ReportType::Short, _index,
                     hidpp20::FeatureID::ROOT, hidpp20::Root::Ping,
                     LOGID_HIDPP_SOFTWARE_ID});
            if(rsp.deviceIndex() != _index) {
                throw InvalidDevice(InvalidDevice::VirtualNode);
            }
        } catch(hidpp10::Error& e) {
            // Ignore
        } catch(std::exception& e) {
            _raw_device->removeEventHandler(_raw_handler);
            throw;
        }

        _raw_device->removeEventHandler(_raw_handler);
    }

    _raw_handler = _raw_device->addEventHandler({
            [index=this->_index](const std::vector<uint8_t>& report)->bool {
                return (report[Offset::Type] == Report::Type::Short ||
                        report[Offset::Type] == Report::Type::Long) &&
                       (report[Offset::DeviceIndex] == index);
            }, [this](const std::vector<uint8_t>& report)->void {
                Report _report(report);
                this->handleEvent(_report);
    } });

    try {
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
            if(std::get<0>(_version) >= 2) {
                try {
                    hidpp20::EssentialDeviceName deviceName(this);
                    _name = deviceName.getName();
                } catch(hidpp20::UnsupportedFeature &e) {
                    _name = _receiver->getDeviceName(_index);
                }
            } else {
                _name = _receiver->getDeviceName(_index);
            }
        }
    } catch(std::exception& e) {
        _raw_device->removeEventHandler(_raw_handler);
        throw;
    }
}

Device::~Device()
{
    _raw_device->removeEventHandler(_raw_handler);
}

Device::EvHandlerId Device::addEventHandler(EventHandler handler)
{
    std::lock_guard<std::mutex> lock(_event_handler_lock);
    _event_handlers.emplace_front(std::move(handler));
    return _event_handlers.cbegin();
}

void Device::removeEventHandler(EvHandlerId id)
{
    std::lock_guard<std::mutex> lock(_event_handler_lock);
    _event_handlers.erase(id);
}

void Device::handleEvent(Report& report)
{
    if(responseReport(report))
        return;

    std::lock_guard<std::mutex> lock(_event_handler_lock);
    for(auto& handler : _event_handlers)
        if(handler.condition(report))
            handler.callback(report);
}

Report Device::sendReport(const Report &report)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    {
        std::lock_guard<std::mutex> sl(_slot_lock);
        _report_slot = {};
    }
    sendReportNoResponse(report);
    std::unique_lock<std::mutex> wait(_resp_wait_lock);
    bool valid = _resp_cv.wait_for(
            wait, io_timeout, [this](){
                std::lock_guard<std::mutex> sl(_slot_lock);
                return _report_slot.has_value();
            });

    if(!valid)
        throw TimeoutError();

    std::lock_guard<std::mutex> sl(_slot_lock);

    {
        Report::Hidpp10Error error{};
        if(_report_slot.value().isError10(&error))
            throw hidpp10::Error(error.error_code);
    }
    {
        Report::Hidpp20Error error{};
        if(_report_slot.value().isError20(&error))
            throw hidpp20::Error(error.error_code);
    }
    return _report_slot.value();
}

bool Device::responseReport(const Report &report)
{
    if(_send_lock.try_lock()) {
        _send_lock.unlock();
        return false;
    }

    // Basic check to see if the report is a response
    if( (report.swId() != LOGID_HIDPP_SOFTWARE_ID)
       && report.subId() < 0x80)
        return false;

    std::lock_guard lock(_slot_lock);
    _report_slot = report;
    _resp_cv.notify_all();
    return true;
}

void Device::sendReportNoResponse(Report report)
{
    reportFixup(report);
    _raw_device->sendReport(report.rawReport());
}

void Device::reportFixup(Report& report)
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
}

std::string Device::name() const
{
    return _name;
}

uint16_t Device::pid() const
{
    return _pid;
}
