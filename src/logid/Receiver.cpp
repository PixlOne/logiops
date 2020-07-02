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

#include <cassert>
#include "Receiver.h"
#include "util/log.h"
#include "backend/hidpp10/Error.h"
#include "backend/hidpp20/Error.h"

using namespace logid;
using namespace logid::backend;

Receiver::Receiver(std::string path) : dj::ReceiverMonitor(path), _path (path)
{
}

void Receiver::addDevice(hidpp::DeviceConnectionEvent event)
{
    try {
        auto dev = _devices.find(event.index);
        if(dev != _devices.end()) {
            if(event.linkEstablished)
                dev->second->wakeup();
            else
                dev->second->sleep();
            return;
        }

        if(!event.linkEstablished)
            return;

        hidpp::Device hidpp_device(receiver(), event);

        auto version = hidpp_device.version();

        if(std::get<0>(version) < 2) {
            logPrintf(INFO, "Unsupported HID++ 1.0 device on %s:%d connected.",
                    _path.c_str(), event.index);
            return;
        }

        std::shared_ptr<Device> device = std::make_shared<Device>(
                receiver()->rawDevice(), event.index);

        _devices.emplace(event.index, device);

    } catch(hidpp10::Error &e) {
        logPrintf(ERROR,
                       "Caught HID++ 1.0 error while trying to initialize "
                       "%s:%d: %s", _path.c_str(), event.index, e.what());
    } catch(hidpp20::Error &e) {
        logPrintf(ERROR, "Caught HID++ 2.0 error while trying to initialize "
                          "%s:%d: %s", _path.c_str(), event.index, e.what());
    }
}

void Receiver::removeDevice(hidpp::DeviceIndex index)
{
    _devices.erase(index);
}