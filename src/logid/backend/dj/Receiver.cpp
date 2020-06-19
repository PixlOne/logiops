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
    return "Invalid receiver";
}

InvalidReceiver::Reason InvalidReceiver::code() const noexcept
{
    return _reason;
}

Receiver::Receiver(std::string path) :
    raw_device (std::make_shared<raw::RawDevice>(path)),
    _hidpp10_device (raw_device, hidpp::DefaultDevice)
{
    if(!supportsDjReports(raw_device->reportDescriptor()))
        throw InvalidReceiver(InvalidReceiver::NoDJReports);


    // Pass all HID++ events on DefaultDevice to handleHidppEvent
    RawEventHandler hidppRawEventHandler;
    hidppRawEventHandler.condition = [this](std::vector<uint8_t>& report)->bool
    {
        return (report[hidpp::Offset::Type] == hidpp::Report::Type::Short ||
                report[hidpp::Offset::Type] == hidpp::Report::Type::Long) &&
               (report[hidpp::Offset::DeviceIndex] == hidpp::DefaultDevice);
    };
    hidppRawEventHandler.callback = [this](std::vector<uint8_t>& report)->void
    {
        hidpp::Report _report(report);
        this->handleHidppEvent(_report);
    };

    // Pass all DJ events with device index to handleHidppEvent
    RawEventHandler djRawEventHandler;
    djRawEventHandler.condition = [this](std::vector<uint8_t>& report)->bool
    {
        return (report[Offset::Type] == Report::Type::Short ||
                report[Offset::Type] == Report::Type::Long);
    };
    djRawEventHandler.callback = [this](std::vector<uint8_t>& report)->void
    {
        Report _report(report);
        this->handleDjEvent(_report);
    };

    raw_device->addEventHandler("RECV_HIDPP", hidppRawEventHandler);
    raw_device->addEventHandler("RECV_DJ", djRawEventHandler);
}

void Receiver::enumerate()
{
    sendDjRequest(hidpp::DefaultDevice, GetPairedDevices,{});
}

Receiver::notification_flags Receiver::getHidppNotifications()
{
    auto response = _hidpp10_device.getRegister(EnableHidppNotifications, {});

    notification_flags flags{};

    flags.device_battery_status = response[0] & (1 << 4);
    flags.receiver_wireless_notifications = response[1] & (1 << 0);
    flags.receiver_software_present = response[1] & (1 << 3);
}

void Receiver::enableHidppNotifications(notification_flags flags)
{
    std::vector<uint8_t> request(3);

    if(flags.device_battery_status)
        request[0] |= (1 << 4);
    if(flags.receiver_wireless_notifications)
        request[1] |= (1 << 0);
    if(flags.receiver_software_present)
        request[1] |= (1 << 3);

    _hidpp10_device.setRegister(EnableHidppNotifications, request);
}

///TODO: Investigate usage
uint8_t Receiver::getConnectionState(hidpp::DeviceIndex index)
{
    auto response = _hidpp10_device.setRegister(ConnectionState, {index});

    return response[0];
}

void Receiver::startPairing(uint8_t timeout)
{
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 1;
    request[1] = hidpp::DefaultDevice;
    request[2] = timeout;

    _hidpp10_device.setRegister(DevicePairing, request);
}

void Receiver::stopPairing()
{
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 2;
    request[1] = hidpp::DefaultDevice;

    _hidpp10_device.setRegister(DevicePairing, request);
}

void Receiver::disconnect(hidpp::DeviceIndex index)
{
    ///TODO: Device number == Device index?
    std::vector<uint8_t> request(3);

    request[0] = 3;
    request[1] = index;

    _hidpp10_device.setRegister(DevicePairing, request);
}

std::map<hidpp::DeviceIndex, uint8_t> Receiver::getDeviceActivity()
{
    auto response = _hidpp10_device.getRegister(DeviceActivity, {});

    std::map<hidpp::DeviceIndex, uint8_t> device_activity;
    for(uint8_t i = hidpp::WirelessDevice1; i <= hidpp::WirelessDevice6; i++)
        device_activity[static_cast<hidpp::DeviceIndex>(i)] = response[i];

    return device_activity;
}

Receiver::pairing_info Receiver::getPairingInfo(hidpp::DeviceIndex index)
{
    std::vector<uint8_t> request(1);
    request[0] = index;
    request[0] += 0x19;

    auto response = _hidpp10_device.getRegister(PairingInfo, request);

    pairing_info info{};
    info.destination_id = response[0];
    info.report_interval = response[1];
    info.pid = response[2];
    info.pid |= (response[3] << 8);
    info.device_type = response[6];

    return info;
}

Receiver::extended_pairing_info
    Receiver::getExtendedPairingInfo(hidpp::DeviceIndex index)
{
    std::vector<uint8_t> request(1);
    request[0] = index;
    request[0] += 0x29;

    auto response = _hidpp10_device.getRegister(PairingInfo, request);

    extended_pairing_info info{};

    info.serial_number = 0;
    for(uint8_t i = 0; i < 4; i++)
        info.serial_number |= (response[i] << 8*i);

    for(uint8_t i = 0; i < 4; i++)
        info.report_types[i] = response[i + 4];

    info.power_switch_location = response[8] & 0xf;

    return info;
}

std::string Receiver::getDeviceName(hidpp::DeviceIndex index)
{
    std::vector<uint8_t> request(1);
    request[0] = index;
    request[0] += 0x39;

    auto response = _hidpp10_device.getRegister(PairingInfo, request);

    uint8_t size = response[0];
    assert(size <= 14);

    std::string name(size, ' ');
    for(std::size_t i = 0; i < size; i++)
        name[i] = response[i + 1];

    return name;
}

hidpp::DeviceIndex Receiver::deviceConnectionEvent(hidpp::Report &report)
{
    assert(report.subId() == DeviceConnection);
    return report.deviceIndex();
}

hidpp::DeviceIndex Receiver::deviceDisconnectionEvent(hidpp::Report& report)
{
    assert(report.subId() == DeviceDisconnection);
    return report.deviceIndex();
}

void Receiver::handleDjEvent(Report& report)
{
    if(report.feature() == DeviceConnection ||
        report.feature() == DeviceDisconnection ||
        report.feature() == ConnectionStatus)
    {
        printf("%s DJ IN: ", raw_device->hidrawPath().c_str());
        for(auto &i: report.rawData())
            printf("%02x ", i);
        printf("\n");
    }
}

void Receiver::handleHidppEvent(hidpp::Report &report)
{
    printf("%s HID++ IN: ", raw_device->hidrawPath().c_str());
    for(auto &i: report.rawReport())
        printf("%02x ", i);
    printf("\n");
}

void Receiver::sendDjRequest(hidpp::DeviceIndex index, uint8_t function,
        const std::vector<uint8_t>&& params)
{
    assert(params.size() <= LongParamLength);

    Report::Type type = params.size() <= ShortParamLength ?
            ReportType::Short : ReportType::Long;

    Report request(type, index, function);

    std::copy(params.begin(), params.end(), request.paramBegin());

    raw_device->sendReportNoResponse(request.rawData());
}

void Receiver::listen()
{
    std::thread{[=]() { raw_device->listen(); }}.detach();
}