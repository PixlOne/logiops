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

#ifndef LOGID_DEVICEMANAGER_H
#define LOGID_DEVICEMANAGER_H

#include <map>
#include <thread>
#include <mutex>

#include "backend/raw/DeviceMonitor.h"
#include "backend/hidpp/Device.h"
#include "Device.h"
#include "Receiver.h"

namespace logid
{

    class DeviceManager : public backend::raw::DeviceMonitor
    {
    public:
        DeviceManager() = default;
    protected:
        void addDevice(std::string path) override;
        void removeDevice(std::string path) override;
    private:

        std::map<std::string, std::shared_ptr<Device>> _devices;
        std::map<std::string, std::shared_ptr<Receiver>> _receivers;
    };

    extern DeviceManager* finder;
}

#endif //LOGID_DEVICEMANAGER_H