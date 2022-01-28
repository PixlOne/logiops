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
#include "Report.h"
#include "Receiver.h"
#include "Error.h"

using namespace logid::backend::dj;
using namespace logid::backend;

InvalidReceiver::InvalidReceiver(Reason reason) : _reason (reason)
{
}

const char* InvalidReceiver::what() const noexcept
{
    switch(_reason) {
    case NoDJReports:
        return "No DJ reports";
    default:
        return "Invalid receiver";
    }
}

InvalidReceiver::Reason InvalidReceiver::code() const noexcept
{
    return _reason;
}

Receiver::Receiver(std::string path,
                   const std::shared_ptr<raw::DeviceMonitor>& monitor,
                   double timeout) :
    _raw_device (std::make_shared<raw::RawDevice>(std::move(path), monitor)),
    _hidpp10_device (_raw_device, hidpp::DefaultDevice, timeout)
{
    if(!supportsDjReports(_raw_device->reportDescriptor()))
        throw InvalidReceiver(InvalidReceiver::NoDJReports);

    // Pass all HID++ events on DefaultDevice to handleHidppEvent
    _raw_hidpp_handler = _raw_device->addEventHandler({
        [](const std::vector<uint8_t>& report)->bool {
            return (report[hidpp::Offset::Type] == hidpp::Report::Type::Short ||
                report[hidpp::Offset::Type] == hidpp::Report::Type::Long);
        },
        [this](const std::vector<uint8_t>& report)->void {
            hidpp::Report _report(report);
            this->_handleHidppEvent(_report);
        }
    });

    // Pass all DJ events with device index to handleDjEvent
    _raw_dj_handler = _raw_device->addEventHandler({
        [](const std::vector<uint8_t>& report)->bool {
            return (report[Offset::Type] == Report::Type::Short ||
            report[Offset::Type] == Report::Type::Long);
            },
            [this](const std::vector<uint8_t>& report)->void {
            Report _report(report);
            this->_handleDjEvent(_report);
        }
    });
}

Receiver::~Receiver()
{
    _raw_device->removeEventHandler(_raw_dj_handler);
    _raw_device->removeEventHandler(_raw_hidpp_handler);
}

void Receiver::enumerateDj()
{
    _sendDjRequest(hidpp::DefaultDevice, GetPairedDevices,{});
}

Receiver::NotificationFlags Receiver::getHidppNotifications()
{
    auto response = _hidpp10_device.getRegister(EnableHidppNotifications, {},
            hidpp::ReportType::Short);

    NotificationFlags flags{};
    flags.deviceBatteryStatus = response[0] & (1 << 4);
    flags.receiverWirelessNotifications = response[1] & (1 << 0);
    flags.receiverSoftwarePresent = response[1] & (1 << 3);

    return flags;
}

void Receiver::enableHidppNotifications(NotificationFlags flags)
{
    std::vector<uint8_t> request(3);

    if(flags.deviceBatteryStatus)
        request[0] |= (1 << 4);
    if(flags.receiverWirelessNotifications)
        request[1] |= 1;
    if(flags.receiverSoftwarePresent)
        request[1] |= (1 << 3);

    _hidpp10_device.setRegister(EnableHidppNotifications, request,
            hidpp::ReportType::Short);
}

void Receiver::enumerateHidpp()
{
    /* This isn't in the documentation but this is how solaar does it
     * All I know is that when (p0 & 2), devices are enumerated
     */
    _hidpp10_device.setRegister(ConnectionState, {2},
            hidpp::ReportType::Short);
}

///TODO: Investigate usage
uint8_t Receiver::getConnectionState(hidpp::DeviceIndex index)
{
    auto response = _hidpp10_device.getRegister(ConnectionState, {index},
            hidpp::ReportType::Short);

    return response[0];
}

void Receiver::startPairing(uint8_t timeout)
{
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 1;
    request[1] = hidpp::DefaultDevice;
    request[2] = timeout;

    _hidpp10_device.setRegister(DevicePairing, request,
            hidpp::ReportType::Short);
}

void Receiver::stopPairing()
{
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 2;
    request[1] = hidpp::DefaultDevice;

    _hidpp10_device.setRegister(DevicePairing, request,
            hidpp::ReportType::Short);
}

void Receiver::disconnect(hidpp::DeviceIndex index)
{
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 3;
    request[1] = index;

    _hidpp10_device.setRegister(DevicePairing, request,
            hidpp::ReportType::Short);
}

std::map<hidpp::DeviceIndex, uint8_t> Receiver::getDeviceActivity()
{
    auto response = _hidpp10_device.getRegister(DeviceActivity, {},
            hidpp::ReportType::Long);

    std::map<hidpp::DeviceIndex, uint8_t> device_activity;
    for(uint8_t i = hidpp::WirelessDevice1; i <= hidpp::WirelessDevice6; i++)
        device_activity[static_cast<hidpp::DeviceIndex>(i)] = response[i];

    return device_activity;
}

struct Receiver::PairingInfo
    Receiver::getPairingInfo(hidpp::DeviceIndex index)
{
    std::vector<uint8_t> request(1);
    request[0] = index;
    request[0] += 0x1f;

    auto response = _hidpp10_device.getRegister(PairingInfo, request,
            hidpp::ReportType::Long);

    struct PairingInfo info{};
    info.destinationId = response[1];
    info.reportInterval = response[2];
    info.pid = response[4];
    info.pid |= (response[3] << 8);
    info.deviceType = static_cast<DeviceType::DeviceType>(response[7]);

    return info;
}

struct Receiver::ExtendedPairingInfo
    Receiver::getExtendedPairingInfo(hidpp::DeviceIndex index)
{
    std::vector<uint8_t> request(1);
    request[0] = index;
    request[0] += 0x2f;

    auto response = _hidpp10_device.getRegister(PairingInfo, request,
            hidpp::ReportType::Long);

    ExtendedPairingInfo info{};

    info.serialNumber = 0;
    for(uint8_t i = 0; i < 4; i++)
        info.serialNumber |= (response[i+1] << 8*i);

    for(uint8_t i = 0; i < 4; i++)
        info.reportTypes[i] = response[i + 5];

    uint8_t psl = response[8] & 0xf;
    if(psl > 0xc)
        info.powerSwitchLocation = PowerSwitchLocation::Reserved;
    else
        info.powerSwitchLocation = static_cast<PowerSwitchLocation>(psl);

    return info;
}

std::string Receiver::getDeviceName(hidpp::DeviceIndex index)
{
    std::vector<uint8_t> request(1);
    request[0] = index;
    request[0] += 0x3f;

    auto response = _hidpp10_device.getRegister(PairingInfo, request,
            hidpp::ReportType::Long);

    uint8_t size = response[1];
    assert(size <= 14);

    std::string name(size, ' ');
    for(std::size_t i = 0; i < size; i++)
        name[i] = response[i + 2];

    return name;
}

hidpp::DeviceIndex Receiver::deviceDisconnectionEvent(const hidpp::Report&
report)
{
    assert(report.subId() == DeviceDisconnection);
    return report.deviceIndex();
}

hidpp::DeviceConnectionEvent Receiver::deviceConnectionEvent(const
        hidpp::Report &report)
{
    assert(report.subId() == DeviceConnection);

    hidpp::DeviceConnectionEvent event{};

    event.index = report.deviceIndex();
    event.unifying = ((report.address() & 0b111) == 0x04);

    event.deviceType = static_cast<DeviceType::DeviceType>(
            report.paramBegin()[0] & 0x0f);
    event.softwarePresent = report.paramBegin()[0] & (1<<4);
    event.encrypted = report.paramBegin()[0] & (1<<5);
    event.linkEstablished = !(report.paramBegin()[0] & (1<<6));
    event.withPayload = report.paramBegin()[0] & (1<<7);
    event.fromTimeoutCheck = false;

    event.pid =(report.paramBegin()[2] << 8);
    event.pid |= report.paramBegin()[1];

    return event;
}

void Receiver::_handleDjEvent(Report& report)
{
    for(auto& handler : _dj_event_handlers)
        if(handler.second->condition(report))
            handler.second->callback(report);
}

void Receiver::_handleHidppEvent(hidpp::Report &report)
{
    for(auto& handler : _hidpp_event_handlers)
        if(handler.second->condition(report))
            handler.second->callback(report);
}

void Receiver::addDjEventHandler(const std::string& nickname,
        const std::shared_ptr<EventHandler>& handler)
{
    assert(_dj_event_handlers.find(nickname) == _dj_event_handlers.end());
    _dj_event_handlers.emplace(nickname, handler);
}

void Receiver::removeDjEventHandler(const std::string &nickname)
{
    _dj_event_handlers.erase(nickname);
}

const std::map<std::string, std::shared_ptr<EventHandler>>&
Receiver::djEventHandlers()
{
    return _dj_event_handlers;
}

void Receiver::addHidppEventHandler(const std::string& nickname,
        const std::shared_ptr<hidpp::EventHandler>& handler)
{
    assert(_hidpp_event_handlers.find(nickname) == _hidpp_event_handlers.end());
    _hidpp_event_handlers.emplace(nickname, handler);
}

void Receiver::removeHidppEventHandler(const std::string &nickname)
{
    _hidpp_event_handlers.erase(nickname);
}

const std::map<std::string, std::shared_ptr<hidpp::EventHandler>>&
Receiver::hidppEventHandlers()
{
    return _hidpp_event_handlers;
}

void Receiver::_sendDjRequest(hidpp::DeviceIndex index, uint8_t function,
        const std::vector<uint8_t>&& params)
{
    assert(params.size() <= LongParamLength);

    Report::Type type = params.size() <= ShortParamLength ?
            ReportType::Short : ReportType::Long;

    Report request(type, index, function);

    std::copy(params.begin(), params.end(), request.paramBegin());

    _raw_device->sendReport(request.rawData());
}

Receiver::ConnectionStatusEvent Receiver::connectionStatusEvent(Report& report)
{
    assert(report.feature() == ConnectionStatus);
    ConnectionStatusEvent event{};
    event.index = report.index();
    event.linkLost = report.paramBegin()[0];
    return event;
}

std::shared_ptr<raw::RawDevice> Receiver::rawDevice() const
{
    return _raw_device;
}