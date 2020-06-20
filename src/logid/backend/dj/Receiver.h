#ifndef LOGID_BACKEND_DJ_RECEIVER_H
#define LOGID_BACKEND_DJ_RECEIVER_H

#include <cstdint>
#include "../raw/RawDevice.h"
#include "Report.h"
#include "../hidpp/Report.h"
#include "../hidpp10/Device.h"

namespace logid {
namespace backend {
namespace dj
{
    class InvalidReceiver : public std::exception
    {
    public:
        enum Reason
        {
            NoDJReports
        };
        explicit InvalidReceiver(Reason reason);
        virtual const char* what() const noexcept;
        Reason code() const noexcept;
    private:
        Reason _reason;
    };

    class Receiver
    {
    public:
        explicit Receiver(std::string path);

        enum DjEvents : uint8_t
        {
            DeviceDisconnection = 0x40,
            DeviceConnection = 0x41,
            ConnectionStatus = 0x42
        };

        enum DjCommands : uint8_t
        {
            SwitchAndKeepAlive = 0x80,
            GetPairedDevices = 0x81
        };

        void enumerate();

        /* The following functions deal with HID++ 1.0 features.
         * While these are not technically DJ functions, it is redundant
         * to have a separate hidpp10::Receiver class for these functions.
         */

        enum HidppEvents : uint8_t
        {
            // These events are identical to their DJ counterparts
            // DeviceDisconnection = 0x40,
            // DeviceConnection = 0x41,
            LockingChange = 0x4a
        };

        enum HidppRegisters : uint8_t
        {
            EnableHidppNotifications = 0x00,
            ConnectionState = 0x02,
            DevicePairing = 0xb2,
            DeviceActivity = 0xb3,
            PairingInfo = 0xb5
        };

        struct notification_flags
        {
            bool device_battery_status;
            bool receiver_wireless_notifications;
            bool receiver_software_present;
        };

        notification_flags getHidppNotifications();
        void enableHidppNotifications(notification_flags flags);

        ///TODO: Understand output of this function
        uint8_t getConnectionState(hidpp::DeviceIndex index);

        void startPairing(uint8_t timeout = 0);
        void stopPairing();
        void disconnect(hidpp::DeviceIndex index);

        std::map<hidpp::DeviceIndex, uint8_t> getDeviceActivity();

        struct PairingInfo
        {
            uint8_t destinationId;
            uint8_t reportInterval;
            uint16_t pid;
            DeviceType::DeviceType deviceType;
        };

        struct ExtendedPairingInfo
        {
            uint32_t serialNumber;
            uint8_t reportTypes[4];
            uint8_t powerSwitchLocation; ///TODO: Make enum
        };

        struct PairingInfo getPairingInfo(hidpp::DeviceIndex index);
        struct ExtendedPairingInfo getExtendedPairingInfo(hidpp::DeviceIndex
                index);

        std::string getDeviceName(hidpp::DeviceIndex index);

        struct DeviceConnectionEvent
        {
            hidpp::DeviceIndex index;
            uint16_t pid;
            DeviceType::DeviceType deviceType;
            bool unifying;
            bool softwarePresent;
            bool encrypted;
            bool linkEstablished;
            bool withPayload;
        };

        static hidpp::DeviceIndex deviceDisconnectionEvent(
                hidpp::Report& report);
        static DeviceConnectionEvent deviceConnectionEvent(
                hidpp::Report& report);

        void handleDjEvent(dj::Report& report);
        void handleHidppEvent(hidpp::Report& report);

        void listen();
        void stopListening();
    private:
        void sendDjRequest(hidpp::DeviceIndex index, uint8_t function,
                const std::vector<uint8_t>&& params);

        std::shared_ptr<raw::RawDevice> raw_device;
        hidpp10::Device _hidpp10_device;
    };
}}}

#endif //LOGID_BACKEND_DJ_RECEIVER_H