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

#ifndef LOGID_RECEIVER_H
#define LOGID_RECEIVER_H

#include <string>
#include "backend/dj/ReceiverMonitor.h"
#include "Device.h"

namespace logid
{
    class Receiver : public backend::dj::ReceiverMonitor
    {
    public:
        explicit Receiver(const std::string& path);
        const std::string& path() const;
        std::shared_ptr<backend::dj::Receiver> rawReceiver();
    protected:
        void addDevice(backend::hidpp::DeviceConnectionEvent event) override;
        void removeDevice(backend::hidpp::DeviceIndex index) override;
    private:
        std::mutex _devices_change;
        std::map<backend::hidpp::DeviceIndex, std::shared_ptr<Device>> _devices;
        std::string _path;
    };
}

#endif //LOGID_RECEIVER_H