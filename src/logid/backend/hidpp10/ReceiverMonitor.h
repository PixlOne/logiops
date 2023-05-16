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

    template<typename T>
    class _receiverMonitorWrapper : public T {
        friend class ReceiverMonitor;

    public:
        template<typename... Args>
        explicit _receiverMonitorWrapper(Args... args) : T(std::forward<Args>(args)...) {}

        template<typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            return std::make_shared<_receiverMonitorWrapper>(std::forward<Args>(args)...);
        }
    };

    static constexpr int max_tries = 5;
    static constexpr int ready_backoff = 250;

    // This class will run on the RawDevice thread,
    class ReceiverMonitor {
    public:
        void enumerate();

        ReceiverMonitor(const ReceiverMonitor&) = delete;

        ReceiverMonitor(ReceiverMonitor&&) = delete;

    protected:
        ReceiverMonitor(const std::string& path,
                        const std::shared_ptr<raw::DeviceMonitor>& monitor,
                        double timeout);


        virtual void addDevice(hidpp::DeviceConnectionEvent event) = 0;

        virtual void removeDevice(hidpp::DeviceIndex index) = 0;

        virtual void pairReady(const hidpp10::DeviceDiscoveryEvent& event,
                               const std::string& passkey) = 0;

        void _startPair(uint8_t timeout = 0);

        void _stopPair();

        void waitForDevice(hidpp::DeviceIndex index);

        [[nodiscard]] std::shared_ptr<Receiver> receiver() const;

    private:
        void _ready();

        void _addHandler(const hidpp::DeviceConnectionEvent& event, int tries = 0);

        void _removeHandler(hidpp::DeviceIndex index);

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

        EventHandlerLock<raw::RawDevice> _connect_ev_handler;

        EventHandlerLock<hidpp::Device> _discover_ev_handler;
        EventHandlerLock<hidpp::Device> _passkey_ev_handler;
        EventHandlerLock<hidpp::Device> _pair_status_handler;

        std::weak_ptr<ReceiverMonitor> _self;

        std::mutex _wait_mutex;
        std::map<hidpp::DeviceIndex, EventHandlerLock<raw::RawDevice>> _waiters;

    public:
        template<typename T, typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            auto receiver_monitor = _receiverMonitorWrapper<T>::make(std::forward<Args>(args)...);
            receiver_monitor->_self = receiver_monitor;
            receiver_monitor->_ready();
            return receiver_monitor;
        }
    };

}

#endif //LOGID_BACKEND_DJ_RECEIVERMONITOR_H