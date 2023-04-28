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

#include <cstdint>
#include <string>
#include "Receiver.h"
#include "../hidpp/defs.h"

namespace logid::backend::dj {
    // This class will run on the RawDevice thread,
    class ReceiverMonitor {
    public:
        ReceiverMonitor(std::string path,
                        const std::shared_ptr<raw::DeviceMonitor>& monitor,
                        double timeout);

        virtual ~ReceiverMonitor();

        void enumerate();

    protected:
        void ready();

        virtual void addDevice(hidpp::DeviceConnectionEvent event) = 0;

        virtual void removeDevice(hidpp::DeviceIndex index) = 0;

        void waitForDevice(hidpp::DeviceIndex index);

        // Internal methods for derived class
        void _pair(uint8_t timeout = 0);

        void _stopPairing();

        void _unpair();

        [[nodiscard]] std::shared_ptr<Receiver> receiver() const;

    private:
        static constexpr const char* ev_handler_name = "receiver_monitor";

        std::shared_ptr<Receiver> _receiver;
    };

}

#endif //LOGID_BACKEND_DJ_RECEIVERMONITOR_H