/*
 * Copyright 2019-2023 PixlOne
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

#include <backend/hidpp10/Receiver.h>
#include <cassert>

using namespace logid::backend::hidpp10;
using namespace logid::backend;

const char* InvalidReceiver::what() const noexcept {
    return "Not a receiver";
}

Receiver::Receiver(const std::string& path, const std::shared_ptr<raw::DeviceMonitor>& monitor,
                   double timeout) : Device(path, hidpp::DefaultDevice, monitor, timeout) {
    // Check if the device is a receiver
    try {
        getNotificationFlags();
    } catch(hidpp10::Error& e) {
        if (e.code() == Error::InvalidAddress)
            throw InvalidReceiver();
    }

    // TODO: is there a better way of checking if this is a bolt receiver?
    _is_bolt = pid() == 0xc548;
}

Receiver::NotificationFlags Receiver::getNotificationFlags() {
    auto response = getRegister(EnableHidppNotifications, {}, hidpp::ReportType::Short);

    NotificationFlags flags{};
    flags.deviceBatteryStatus = response[0] & (1 << 4);
    flags.receiverWirelessNotifications = response[1] & (1 << 0);
    flags.receiverSoftwarePresent = response[1] & (1 << 3);

    return flags;
}

void Receiver::setNotifications(NotificationFlags flags) {
    std::vector<uint8_t> request(3);

    if (flags.deviceBatteryStatus)
        request[0] |= (1 << 4);
    if (flags.receiverWirelessNotifications)
        request[1] |= 1;
    if (flags.receiverSoftwarePresent)
        request[1] |= (1 << 3);

    setRegister(EnableHidppNotifications, request, hidpp::ReportType::Short);
}

void Receiver::enumerate() {
    setRegister(ConnectionState, {2}, hidpp::ReportType::Short);
}

///TODO: Investigate usage
uint8_t Receiver::getConnectionState(hidpp::DeviceIndex index) {
    auto response = getRegister(ConnectionState, {index}, hidpp::ReportType::Short);

    return response[0];
}

void Receiver::startPairing(uint8_t timeout) {
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 1;
    request[1] = hidpp::DefaultDevice;
    request[2] = timeout;

    setRegister(DevicePairing, request, hidpp::ReportType::Short);
}

void Receiver::stopPairing() {
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 2;
    request[1] = hidpp::DefaultDevice;

    setRegister(DevicePairing, request, hidpp::ReportType::Short);
}

void Receiver::disconnect(hidpp::DeviceIndex index) {
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 3;
    request[1] = index;

    setRegister(DevicePairing, request, hidpp::ReportType::Short);
}

std::map<hidpp::DeviceIndex, uint8_t> Receiver::getDeviceActivity() {
    auto response = getRegister(DeviceActivity, {}, hidpp::ReportType::Long);

    std::map<hidpp::DeviceIndex, uint8_t> device_activity;
    for (uint8_t i = hidpp::WirelessDevice1; i <= hidpp::WirelessDevice6; i++)
        device_activity[static_cast<hidpp::DeviceIndex>(i)] = response[i];

    return device_activity;
}

struct Receiver::PairingInfo
Receiver::getPairingInfo(hidpp::DeviceIndex index) {
    std::vector<uint8_t> request(1);
    request[0] = index;
    if (!_is_bolt)
        request[0] += 0x1f;

    auto response = getRegister(PairingInfo, request, hidpp::ReportType::Long);

    struct PairingInfo info{};
    info.destinationId = response[1];
    info.reportInterval = response[2];
    info.pid = response[4];
    info.pid |= (response[3] << 8);
    info.deviceType = static_cast<hidpp::DeviceType>(response[7]);

    return info;
}

struct Receiver::ExtendedPairingInfo
Receiver::getExtendedPairingInfo(hidpp::DeviceIndex index) {
    std::vector<uint8_t> request(1);
    request[0] = index;
    if (_is_bolt)
        request[0] += 0x50;
    else
        request[0] += 0x2f;

    auto response = getRegister(PairingInfo, request, hidpp::ReportType::Long);

    ExtendedPairingInfo info{};

    info.serialNumber = 0;
    for (uint8_t i = 0; i < 4; i++)
        info.serialNumber |= (response[i + 1] << 8 * i);

    for (uint8_t i = 0; i < 4; i++)
        info.reportTypes[i] = response[i + 5];

    uint8_t psl = response[8] & 0xf;
    if (psl > 0xc)
        info.powerSwitchLocation = PowerSwitchLocation::Reserved;
    else
        info.powerSwitchLocation = static_cast<PowerSwitchLocation>(psl);

    return info;
}

std::string Receiver::getDeviceName(hidpp::DeviceIndex index) {
    std::vector<uint8_t> request(2);
    std::string name;
    request[0] = index;
    if (_is_bolt) {
        /* Undocumented, deduced the following
         * param 1 refers to part of string, 1-indexed
         *
         * response at 0x01 is [reg] [param 1] [size] [str...]
         * response at 0x02-... is [next part of str...]
         */
        request[1] = 0x01;

        auto resp = getRegister(PairingInfo, request, hidpp::ReportType::Long);
        const uint8_t size = resp[2];
        const uint8_t chunk_size = resp.size() - 3;
        const uint8_t chunks = size/chunk_size + (size % chunk_size ? 1 : 0);

        name.resize(size, ' ');
        for (int i = 0; i < chunks; ++i) {
            for (int j = 0; j < chunk_size; ++j) {
                name[i*chunk_size + j] = (char)resp[j + 3];
            }

            if (i < chunks - 1) {
                request[1] = i+1;
                resp = getRegister(PairingInfo, request, hidpp::ReportType::Long);
            }
        }

    } else {
        request[0] += 0x3f;

        auto response = getRegister(PairingInfo, request, hidpp::ReportType::Long);

        const uint8_t size = response[1];

        name.resize(size, ' ');
        for (std::size_t i = 0; i < size && i + 2 < response.size(); i++)
            name[i] = (char) (response[i + 2]);

    }

    return name;
}

hidpp::DeviceIndex Receiver::deviceDisconnectionEvent(const hidpp::Report& report) {
    assert(report.subId() == DeviceDisconnection);
    return report.deviceIndex();
}

hidpp::DeviceConnectionEvent Receiver::deviceConnectionEvent(const hidpp::Report& report) {
    assert(report.subId() == DeviceConnection);

    hidpp::DeviceConnectionEvent event{};

    event.index = report.deviceIndex();
    event.unifying = ((report.address() & 0b111) == 0x04);

    event.deviceType = static_cast<hidpp::DeviceType>(report.paramBegin()[0] & 0x0f);
    event.softwarePresent = report.paramBegin()[0] & (1 << 4);
    event.encrypted = report.paramBegin()[0] & (1 << 5);
    event.linkEstablished = !(report.paramBegin()[0] & (1 << 6));
    event.withPayload = report.paramBegin()[0] & (1 << 7);
    event.fromTimeoutCheck = false;

    event.pid = (report.paramBegin()[2] << 8);
    event.pid |= report.paramBegin()[1];

    return event;
}
