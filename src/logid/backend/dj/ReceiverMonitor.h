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

#ifndef LOGID_BACKEND_DJ_RECEIVERMONITOR_H
#define LOGID_BACKEND_DJ_RECEIVERMONITOR_H

#include <cstdint>
#include <string>
#include "Receiver.h"
#include "../hidpp/defs.h"

namespace logid {
namespace backend {
namespace dj
{
    // This class will run on the RawDevice thread,
    class ReceiverMonitor
    {
    public:
        explicit ReceiverMonitor(std::string path);
        virtual ~ReceiverMonitor();

        void enumerate();
        void run();
        void stop();

    protected:
        virtual void addDevice(hidpp::DeviceConnectionEvent event) = 0;
        virtual void removeDevice(hidpp::DeviceIndex index) = 0;

        void waitForDevice(hidpp::DeviceIndex index);

        // Internal methods for derived class
        void _pair(uint8_t timeout = 0);
        void _stopPairing();

        void _unpair();

        std::shared_ptr<Receiver> receiver() const;
    private:
        std::shared_ptr<Receiver> _receiver;
    };

}}}

#endif //LOGID_BACKEND_DJ_RECEIVERMONITOR_H