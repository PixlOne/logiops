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

#include "Device.h"
#include "util.h"

using namespace logid;

IPC::Device::Device(DBus::Connection &connection, logid::Device* device): DBus::ObjectAdaptor(
        connection, IPC::toDBusPath(device)), _device(device)
{
    Name = device->name;
    std::vector<std::string> features;
    /// TODO: Set features
    Features = features;
    DeviceID = device->pid;
}

void IPC::Device::GetInfo(const std::string& device)
{

}
