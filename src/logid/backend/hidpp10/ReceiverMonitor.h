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

#ifndef LOGID_BACKEND_DJ_RECEIVERMONITOR_H
#define LOGID_BACKEND_DJ_RECEIVERMONITOR_H

#include <backend/hidpp10/Receiver.h>
#include <backend/hidpp/defs.h>
#include <cstdint>
#include <string>

namespace logid::backend::hidpp10 {
    // This class will run on the RawDevice thread,
    class ReceiverMonitor {
    public:
        ReceiverMonitor(const std::string& path,
                        const std::shared_ptr<raw::DeviceMonitor>& monitor,
                        double timeout);

        virtual ~ReceiverMonitor();

        void enumerate();

    protected:
        void ready();

        virtual void addDevice(hidpp::DeviceConnectionEvent event) = 0;

        virtual void removeDevice(hidpp::DeviceIndex index) = 0;

        virtual void pairReady(const hidpp10::DeviceDiscoveryEvent& event,
                               const std::string& passkey) = 0;

        void _startPair(uint8_t timeout = 0);

        void _stopPair();

        void waitForDevice(hidpp::DeviceIndex index);

        [[nodiscard]] std::shared_ptr<Receiver> receiver() const;

    private:
        std::shared_ptr<Receiver> _receiver;

        enum PairState {
            NotPairing,
            Discovering,
            FindingPasskey,
            Pairing,
        };

        std::mutex _pair_mutex;
        DeviceDiscoveryEvent _discovery_event;
        PairState _pair_state = NotPairing;

        std::optional<raw::RawDevice::EvHandlerId> _connect_ev_handler;

        std::optional<hidpp::Device::EvHandlerId> _discover_ev_handler;
        std::optional<hidpp::Device::EvHandlerId> _passkey_ev_handler;
        std::optional<hidpp::Device::EvHandlerId> _pair_status_handler;
    };

}

#endif //LOGID_BACKEND_DJ_RECEIVERMONITOR_H