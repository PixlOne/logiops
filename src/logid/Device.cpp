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

#include "util/log.h"
#include "Device.h"

using namespace logid;
using namespace logid::backend;

Device::Device(std::string path, backend::hidpp::DeviceIndex index) :
    _hidpp20 (path, index), _path (std::move(path)), _index (index)
{
    logPrintf(DEBUG, "logid::Device created on %s:%d", _path.c_str(), _index);
}

Device::Device(const std::shared_ptr<backend::raw::RawDevice>& raw_device,
        hidpp::DeviceIndex index) : _hidpp20(raw_device, index), _path
        (raw_device->hidrawPath()), _index (index)
{
    logPrintf(DEBUG, "logid::Device created on %s:%d", _path.c_str(), _index);
}

void Device::sleep()
{
    logPrintf(INFO, "%s:%d fell asleep.", _path.c_str(), _index);
}

void Device::wakeup()
{
    logPrintf(INFO, "%s:%d woke up.", _path.c_str(), _index);
}
