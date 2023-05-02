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

#ifndef LOGID_BACKEND_DJ_RECEIVER_H
#define LOGID_BACKEND_DJ_RECEIVER_H

#include <cstdint>
#include <backend/hidpp10/Device.h>

namespace logid::backend::hidpp {
    enum DeviceType : uint8_t {
        DeviceUnknown = 0x00,
        DeviceKeyboard = 0x01,
        DeviceMouse = 0x02,
        DeviceNumpad = 0x03,
        DevicePresenter = 0x04,
        /* 0x05-0x07 is reserved */
        DeviceTrackball = 0x08,
        DeviceTouchpad = 0x09
    };

    struct DeviceConnectionEvent {
        DeviceIndex index{};
        uint16_t pid{};
        DeviceType deviceType = DeviceUnknown;
        bool unifying{};
        bool softwarePresent{};
        bool encrypted{};
        bool linkEstablished{};
        bool withPayload{};
        bool fromTimeoutCheck = false; // Fake field
    };
}

namespace logid::backend::hidpp10 {

    struct DeviceDiscoveryEvent {
        hidpp::DeviceType deviceType = hidpp::DeviceUnknown;
        uint8_t seq_num{};
        uint64_t address{};
        uint16_t pid{};
        uint8_t authentication{};
        std::string name;
    };

    enum PairingError : uint8_t {
        NoPairingError = 0x00,
        Timeout = 0x01,
        UnsupportedDevice = 0x02,
        TooManyDevices = 0x03,
        /* Errors 0x04-0x05 are reserved */
        ConnectionSeqTimeout = 0x06,
    };

    struct PairStatusEvent {
        bool pairing{};
        PairingError error = NoPairingError;
    };

    struct BoltPairStatusEvent {
        bool pairing{};
        uint8_t error;
    };

    struct DiscoveryStatusEvent {
        bool discovering{};
        uint8_t error{}; // don't know the error codes
    };

    class InvalidReceiver : public std::exception {
    public:
        [[nodiscard]] const char* what() const noexcept override;
    };

    class Receiver : public Device {
    public:

        /* The following functions deal with HID++ 1.0 features.
         * While these are not technically DJ functions, it is redundant
         * to have a separate hidpp10::Receiver class for these functions.
         */

        enum Events : uint8_t {
            // These events are identical to their DJ counterparts
            DeviceDisconnection = 0x40,
            DeviceConnection = 0x41,
            PairStatus = 0x4a,
            PasskeyRequest = 0x4d,
            DeviceDiscovered = 0x4f,
            DiscoveryStatus = 0x53,
            BoltPairStatus = 0x54,
        };

        enum Registers : uint8_t {
            EnableHidppNotifications = 0x00,
            ConnectionState = 0x02,
            DevicePairing = 0xb2,
            DeviceActivity = 0xb3,
            PairingInfo = 0xb5,
            BoltDeviceDiscovery = 0xc0,
            BoltDevicePairing = 0xc1,
        };

        struct NotificationFlags {
            bool deviceBatteryStatus;
            bool receiverWirelessNotifications;
            bool receiverSoftwarePresent;
        };

        NotificationFlags getNotificationFlags();

        void setNotifications(NotificationFlags flags);

        void enumerate();

        uint8_t getConnectionState(hidpp::DeviceIndex index);

        void startPairing(uint8_t timeout = 0);

        void startBoltPairing(const DeviceDiscoveryEvent& discovery);

        void stopPairing();

        void startDiscover(uint8_t timeout = 0);

        void stopDiscover();

        void disconnect(hidpp::DeviceIndex index);

        std::map<hidpp::DeviceIndex, uint8_t> getDeviceActivity();

        [[nodiscard]] bool bolt() const;

        struct PairingInfo {
            uint8_t destinationId;
            uint8_t reportInterval;
            uint16_t pid;
            hidpp::DeviceType deviceType;
        };

        enum class PowerSwitchLocation : uint8_t {
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

        struct ExtendedPairingInfo {
            uint32_t serialNumber;
            uint8_t reportTypes[4];
            PowerSwitchLocation powerSwitchLocation;
        };

        struct PairingInfo getPairingInfo(hidpp::DeviceIndex index);

        struct ExtendedPairingInfo getExtendedPairingInfo(hidpp::DeviceIndex index);

        std::string getDeviceName(hidpp::DeviceIndex index);

        static hidpp::DeviceIndex deviceDisconnectionEvent(const hidpp::Report& report);

        static hidpp::DeviceConnectionEvent deviceConnectionEvent(const hidpp::Report& report);

        static PairStatusEvent pairStatusEvent(const hidpp::Report& report);

        static BoltPairStatusEvent boltPairStatusEvent(const hidpp::Report& report);

        static DiscoveryStatusEvent discoveryStatusEvent(const hidpp::Report& report);

        static bool fillDeviceDiscoveryEvent(DeviceDiscoveryEvent& event,
                                             const hidpp::Report& report);

        static std::string passkeyEvent(const hidpp::Report& report);

    protected:
        Receiver(const std::string& path,
                 const std::shared_ptr<raw::DeviceMonitor>& monitor,
                 double timeout);

    private:
        void _receiverCheck();

        bool _is_bolt = false;

    public:
        template <typename... Args>
        static std::shared_ptr<Receiver> make(Args... args) {
            auto receiver = makeDerived<Receiver>(std::forward<Args>(args)...);

            receiver->_receiverCheck();

            return receiver;
        }
    };
}

#endif //LOGID_BACKEND_DJ_RECEIVER_H