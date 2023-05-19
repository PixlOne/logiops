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
}

void Receiver::_receiverCheck() {
    // Check if the device is a receiver
    try {
        getNotificationFlags();
    } catch (hidpp10::Error& e) {
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
    setRegisterNoResponse(ConnectionState, {2}, hidpp::ReportType::Short);
}

///TODO: Investigate usage
uint8_t Receiver::getConnectionState(hidpp::DeviceIndex index) {
    auto response = getRegister(ConnectionState, {index}, hidpp::ReportType::Short);

    return response[0];
}

void Receiver::startPairing(uint8_t timeout) {
    std::vector<uint8_t> request(3);

    if (_is_bolt)
        throw std::invalid_argument("unifying pairing on bolt");

    request[0] = 1;
    request[1] = hidpp::DefaultDevice;
    request[2] = timeout;

    if (_is_bolt) {
        setRegister(BoltDevicePairing, request, hidpp::ReportType::Long);
    } else {
        setRegister(DevicePairing, request, hidpp::ReportType::Short);
    }
}

// bolt pairing request from solaar
void Receiver::startBoltPairing(const DeviceDiscoveryEvent& discovery) {
    std::vector<uint8_t> request(10);

    request[0] = 1; // start pair
    request[1] = 0; // slot, from solaar. what does this mean?

    for(int i = 0; i < 6; ++i)
        request[2 + i]  = (discovery.address >> (i*8)) & 0xff;

    request[8] = discovery.authentication;

    // TODO: what does entropy do?
    request[9] = (discovery.deviceType == hidpp::DeviceKeyboard) ? 10 : 20;

    setRegister(BoltDevicePairing, request, hidpp::ReportType::Long);
}

void Receiver::stopPairing() {
    std::vector<uint8_t> request(3);

    request[0] = 2;
    request[1] = hidpp::DefaultDevice;

    if (_is_bolt)
        setRegister(BoltDevicePairing, request, hidpp::ReportType::Long);
    else
        setRegister(DevicePairing, request, hidpp::ReportType::Short);
}

void Receiver::startDiscover(uint8_t timeout) {
    std::vector<uint8_t> request = {timeout, 1};

    if (!_is_bolt)
        throw std::invalid_argument("not a bolt receiver");

    setRegister(BoltDeviceDiscovery, request, hidpp::ReportType::Short);
}

void Receiver::stopDiscover() {
    std::vector<uint8_t> request = {0, 2};

    if (!_is_bolt)
        throw std::invalid_argument("not a bolt receiver");

    setRegister(BoltDeviceDiscovery, request, hidpp::ReportType::Short);
}

void Receiver::disconnect(hidpp::DeviceIndex index) {
    std::vector<uint8_t> request(2);

    request[0] = _is_bolt ? 3 : 2;
    request[1] = index;

    if (_is_bolt)
        setRegister(BoltDevicePairing, request, hidpp::ReportType::Long);
    else
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
    if (_is_bolt)
        request[0] += 0x50;
    else
        request[0] += 0x1f;

    auto response = getRegister(PairingInfo, request, hidpp::ReportType::Long);

    struct PairingInfo info{};
    if (_is_bolt) {
        info = {
                .destinationId = 0, // no longer given?
                .reportInterval = 0, // no longer given?
                .pid = (uint16_t) ((response[3] << 8) | response[2]),
                .deviceType = static_cast<hidpp::DeviceType>(response[1])
        };
    } else {
        info = {
                .destinationId = response[1],
                .reportInterval = response[2],
                .pid = (uint16_t) ((response[3] << 8) | response[4]),
                .deviceType = static_cast<hidpp::DeviceType>(response[7])
        };
    }


    return info;
}

struct Receiver::ExtendedPairingInfo
Receiver::getExtendedPairingInfo(hidpp::DeviceIndex index) {
    const int device_num_offset = _is_bolt ? 0x50 : 0x2f;
    const int serial_num_offset = _is_bolt ? 4 : 1;
    const int report_offset = _is_bolt ? 8 : 5;
    const int psl_offset = _is_bolt ? 12 : 8;

    std::vector<uint8_t> request(1, index + device_num_offset);

    auto response = getRegister(PairingInfo, request, hidpp::ReportType::Long);

    ExtendedPairingInfo info{};

    info.serialNumber = 0;
    for (uint8_t i = 0; i < 4; i++)
        info.serialNumber |= (response[i + serial_num_offset] << 8 * i);

    for (uint8_t i = 0; i < 4; i++)
        info.reportTypes[i] = response[i + report_offset];

    uint8_t psl = response[psl_offset] & 0xf;
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
        request[0] += 0x60;
        request[1] = 0x01;

        auto resp = getRegister(PairingInfo, request, hidpp::ReportType::Long);
        const uint8_t size = resp[2];
        const uint8_t chunk_size = resp.size() - 3;
        const uint8_t chunks = size / chunk_size + (size % chunk_size ? 1 : 0);

        name.resize(size, ' ');
        for (int i = 0; i < chunks; ++i) {
            for (int j = 0; j < chunk_size; ++j) {
                name[i * chunk_size + j] = (char) resp[j + 3];
            }

            if (i < chunks - 1) {
                request[1] = i + 1;
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

    auto data = report.paramBegin();

    return {
            .index = report.deviceIndex(),
            .pid = (uint16_t) ((data[2] << 8) | data[1]),
            .deviceType = static_cast<hidpp::DeviceType>(data[0] & 0x0f),
            .unifying = ((report.address() & 0b111) == 0x04),
            .softwarePresent = bool(data[0] & (1 << 4)),
            .encrypted = (bool) (data[0] & (1 << 5)),
            .linkEstablished = !(data[0] & (1 << 6)),
            .withPayload = (bool) (data[0] & (1 << 7)),

            .fromTimeoutCheck = false,
    };
}

bool Receiver::fillDeviceDiscoveryEvent(DeviceDiscoveryEvent& event,
                                        const hidpp::Report& report) {
    assert(report.subId() == DeviceDiscovered);

    auto data = report.paramBegin();

    if (data[1] == 0) {
        // device discovery event

        uint64_t address = 0 ;
        for (int i = 0; i < 6; ++i)
            address |= ((uint64_t)(data[6 + i]) << (8*i));

        event.deviceType = static_cast<hidpp::DeviceType>(data[3]);
        event.pid = (data[5] << 8) | data[4];
        event.address = address;
        event.authentication = data[14];
        event.seq_num = report.address();
        event.name = "";

        return false;
    } else {
        /* bad sequence, do not continue */
        if (event.seq_num != report.address())
            return false;

        const int block_size = hidpp::LongParamLength - 3;

        if (data[1] == 1) {
            event.name.resize(data[2], ' ');
        }

        for(int i = 0; i < block_size; ++i) {
            const size_t j = (data[1]-1)*block_size + i;
            if (j < event.name.size()) {
                event.name[j] = (char)data[i + 3];
            } else {
                return true;
            }
        }

        return false;
    }
}

PairStatusEvent Receiver::pairStatusEvent(const hidpp::Report& report) {
    assert(report.subId() == PairStatus);

    return {
        .pairing = (bool)(report.paramBegin()[0] & 1),
        .error = static_cast<PairingError>(report.paramBegin()[1])
    };
}

BoltPairStatusEvent Receiver::boltPairStatusEvent(const hidpp::Report& report) {
    assert(report.subId() == BoltPairStatus);

    return {
            .pairing = report.address() == 0,
            .error = report.paramBegin()[1]
    };
}

DiscoveryStatusEvent Receiver::discoveryStatusEvent(const hidpp::Report& report) {
    assert(report.subId() == DiscoveryStatus);

    return {
        .discovering = report.address() == 0,
        .error = report.paramBegin()[1]
    };
}

std::string Receiver::passkeyEvent(const hidpp::Report& report) {
    assert(report.subId() == PasskeyRequest);

    return {report.paramBegin(), report.paramBegin() + 6};
}

bool Receiver::bolt() const {
    return _is_bolt;
}
