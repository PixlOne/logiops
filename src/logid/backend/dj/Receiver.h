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
    struct EventHandler
    {
        std::function<bool(Report&)> condition;
        std::function<void(Report&)> callback;
    };

    class InvalidReceiver : public std::exception
    {
    public:
        enum Reason
        {
            NoDJReports
        };
        explicit InvalidReceiver(Reason reason);
        const char* what() const noexcept override;
        Reason code() const noexcept;
    private:
        Reason _reason;
    };

    class Receiver final
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

        void enumerateDj();

        struct ConnectionStatusEvent
        {
            hidpp::DeviceIndex index;
            bool linkLost;
        };

        ConnectionStatusEvent connectionStatusEvent(dj::Report& report);

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

        struct NotificationFlags
        {
            bool deviceBatteryStatus;
            bool receiverWirelessNotifications;
            bool receiverSoftwarePresent;
        };

        NotificationFlags getHidppNotifications();
        void enableHidppNotifications(NotificationFlags flags);

        void enumerateHidpp();
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

        enum class PowerSwitchLocation : uint8_t
        {
            Reserved = 0x0,
            Base = 0x1,
            TopCase = 0x2,
            TopRightEdge = 0x3,
            Other = 0x4,
            TopLeft = 0x5,
            BottomLeft = 0x6,
            TopRight = 0x7,
            BottomRight = 0x8,
            TopEdge = 0x9,
            RightEdge = 0xa,
            LeftEdge = 0xb,
            BottomEdge = 0xc
        };

        struct ExtendedPairingInfo
        {
            uint32_t serialNumber;
            uint8_t reportTypes[4];
            PowerSwitchLocation powerSwitchLocation;
        };

        struct PairingInfo getPairingInfo(hidpp::DeviceIndex index);
        struct ExtendedPairingInfo getExtendedPairingInfo(hidpp::DeviceIndex
                index);

        std::string getDeviceName(hidpp::DeviceIndex index);

        static hidpp::DeviceIndex deviceDisconnectionEvent(
                const hidpp::Report& report);
        static hidpp::DeviceConnectionEvent deviceConnectionEvent(
                const hidpp::Report& report);

        void listen();
        void stopListening();

        void addDjEventHandler(const std::string& nickname,
                const std::shared_ptr<EventHandler>& handler);
        void removeDjEventHandler(const std::string& nickname);
        const std::map<std::string, std::shared_ptr<EventHandler>>&
        djEventHandlers();

        void addHidppEventHandler(const std::string& nickname,
                const std::shared_ptr<hidpp::EventHandler>& handler);
        void removeHidppEventHandler(const std::string& nickname);
        const std::map<std::string, std::shared_ptr<hidpp::EventHandler>>&
            hidppEventHandlers();

        std::shared_ptr<raw::RawDevice> rawDevice() const;
    private:
        void _sendDjRequest(hidpp::DeviceIndex index, uint8_t function,
                const std::vector<uint8_t>&& params);

        void _handleDjEvent(dj::Report& report);
        void _handleHidppEvent(hidpp::Report& report);

        std::map<std::string, std::shared_ptr<EventHandler>>
                _dj_event_handlers;
        std::map<std::string, std::shared_ptr<hidpp::EventHandler>>
                _hidpp_event_handlers;

        std::shared_ptr<raw::RawDevice> _raw_device;
        hidpp10::Device _hidpp10_device;
    };
}

namespace hidpp
{
    struct DeviceConnectionEvent
    {
        hidpp::DeviceIndex index;
        uint16_t pid;
        dj::DeviceType::DeviceType deviceType;
        bool unifying;
        bool softwarePresent;
        bool encrypted;
        bool linkEstablished;
        bool withPayload;
        bool fromTimeoutCheck = false; // Fake field
    };
}}}

#endif //LOGID_BACKEND_DJ_RECEIVER_H