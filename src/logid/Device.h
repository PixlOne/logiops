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

#ifndef LOGID_DEVICE_H
#define LOGID_DEVICE_H

#include "backend/hidpp/defs.h"
#include "backend/hidpp20/Device.h"
#include "backend/hidpp20/Feature.h"
#include "features/DeviceFeature.h"
#include "Configuration.h"

namespace logid
{
    class Device;

    class DeviceConfig
    {
    public:
        DeviceConfig(const std::shared_ptr<Configuration>& config, Device*
        device);
        libconfig::Setting& getSetting(std::string path);
    private:
        Device* _device;
        std::string _root_setting;
        std::shared_ptr<Configuration> _config;
    };

    /* TODO: Implement HID++ 1.0 support
     * Currently, the logid::Device class has a hardcoded requirement
     * for an HID++ 2.0 device.
     */
    class Device
    {
    public:
        Device(std::string path, backend::hidpp::DeviceIndex index);
        Device(const std::shared_ptr<backend::raw::RawDevice>& raw_device,
                backend::hidpp::DeviceIndex index);

        std::string name();
        uint16_t pid();

        DeviceConfig& config();
        backend::hidpp20::Device& hidpp20();

        void wakeup();
        void sleep();
    private:
        void _init();

        /* Adds a feature without calling an error if unsupported */
        template<typename T>
        void _addFeature()
        {
            try {
                _features.push_back(std::make_shared<T>(this));
            } catch (backend::hidpp20::UnsupportedFeature& e) {
            }
        }

        backend::hidpp20::Device _hidpp20;
        std::string _path;
        backend::hidpp::DeviceIndex _index;
        std::vector<std::shared_ptr<features::DeviceFeature>> _features;
        DeviceConfig _config;
    };
}

#endif //LOGID_DEVICE_H
